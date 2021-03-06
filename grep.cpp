// Copyright 2017 Global Phasing Ltd.

#include "gemmi/cif.hpp"
#include "gemmi/cifgz.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <optionparser.h>

#define EXE_NAME "gemmi-grep"

struct Arg: public option::Arg {
  static option::ArgStatus Int(const option::Option& option, bool msg) {
    if (option.arg) {
      char* endptr = nullptr;
      std::strtol(option.arg, &endptr, 10);
      if (endptr != option.arg && *endptr == '\0')
        return option::ARG_OK;
    }
    if (msg)
      fprintf(stderr, "Option '%s' requires a numeric argument\n", option.name);
    return option::ARG_ILLEGAL;
  }
};

enum OptionIndex { Unknown, Help, MaxCount, WithFileName, NoBlockName, WithTag,
                   Summarize, MatchingFiles, NonMatchingFiles, Count };

const option::Descriptor usage[] = {
  { Unknown, 0, "", "", Arg::None,
    "Usage: " EXE_NAME " [options] TAG FILE_OR_DIR[...]\n"
    "Search for TAG in CIF files."
    "\n\nOptions:" },
  { Help, 0, "h", "help", Arg::None,
    "  -h, --help  \tprint usage and exit" },
  { MaxCount, 0, "m", "max-count", Arg::Int,
    "  -m, --max-count=NUM  \tprint max NUM values per block (default: 10)" },
  { WithFileName, 0, "H", "with-filename", Arg::None,
    "  -H, --with-filename  \tprint the file name for each match" },
  { NoBlockName, 0, "b", "no-blockname", Arg::None,
    "  -b, --no-blockname  \tsuppress the block name on output" },
  { WithTag, 0, "t", "with-tag", Arg::None,
    "  -t, --with-tag  \tprint the tag name for each match" },
  { MatchingFiles, 0, "l", "files-with-tag", Arg::None,
    "  -l, --files-with-tag  \tprint only names of files with the tag" },
  { NonMatchingFiles, 0, "L", "files-without-tag", Arg::None,
    "  -L, --files-without-tag  \tprint only names of files without the tag" },
  { Count, 0, "c", "count", Arg::None,
    "  -c, --count  \tprint only a count of matching lines per file" },
  { Summarize, 0, "s", "summarize", Arg::None,
    "  -s, --summarize  \tdisplay only statistics" },
  { 0, 0, 0, 0, 0, 0 }
};

struct Parameters {
  std::string search_tag;
  int max_count = 10;
  bool with_filename = false;
  bool with_blockname = true;
  bool with_tag = false;
  bool summarize = false;
  bool only_filenames = false;
  bool inverse = false;
  bool print_count = false;
  // working parameters
  const char* path = "";
  std::string block_name;
  bool match_value = false;
  int match_column = -1;
  int table_width = 0;
  int column = 0;
  int counter = 0;
};

static void process_match(const std::string& value, Parameters& par) {
  //if (par.only_filenames)
  //  throw ...
  if (par.print_count) {
    par.counter++;
    return;
  }
  if (par.with_filename)
    printf("%s: ", par.path);
  if (par.with_blockname)
    printf("%s: ", par.block_name.c_str());
  if (par.with_tag)
    printf("[%s] ", par.search_tag.c_str());
  printf(" %s\n", value.c_str());
  //if (++par.counter == par.max_count)
  //  throw ...
}

static void finish_processing(Parameters& par) {
  if (par.print_count) {
    if (par.with_filename)
      printf("%s: ", par.path);
    if (par.with_blockname)
      printf("%s: ", par.block_name.c_str());
    printf(" %d\n", par.counter);
    par.counter = 0;
  }
  // throw
}

namespace pegtl = tao::pegtl;
namespace rules = gemmi::cif::rules;
namespace cif = gemmi::cif;

template<typename Rule> struct Search : pegtl::nothing<Rule> {};

template<> struct Search<rules::datablockname> {
  template<typename Input> static void apply(const Input& in, Parameters& p) {
    p.block_name = in.string();
  }
};
template<> struct Search<rules::str_global> {
  template<typename Input> static void apply(const Input&, Parameters& p) {
    p.block_name = "global_";
  }
};
template<> struct Search<rules::tag> {
  template<typename Input> static void apply(const Input& in, Parameters& p) {
    if (p.search_tag == in.string())
      p.match_value = true;
  }
};
template<> struct Search<rules::value> {
  template<typename Input> static void apply(const Input& in, Parameters& p) {
    if (p.match_value) {
      process_match(cif::as_string(in.string()), p);
      finish_processing(p);
      p.match_value = false;
    }
  }
};
template<> struct Search<rules::str_loop> {
  template<typename Input> static void apply(const Input&, Parameters& p) {
    p.table_width = 0;
  }
};
template<> struct Search<rules::loop_tag> {
  template<typename Input> static void apply(const Input& in, Parameters& p) {
    if (p.search_tag == in.string()) {
      p.match_column = p.table_width;
      p.column = 0;
    }
    p.table_width++;
  }
};
template<> struct Search<rules::loop_end> {
  template<typename Input> static void apply(const Input&, Parameters& p) {
    if (p.match_column != -1) {
      finish_processing(p);
      p.match_column = -1;
    }
  }
};
template<> struct Search<rules::loop_value> {
  template<typename Input> static void apply(const Input& in, Parameters& p) {
    if (p.match_column != -1) {
      if (p.column == p.match_column)
        process_match(cif::as_string(in.string()), p);
      p.column++;
      if (p.column == p.table_width)
        p.column = 0;
    }
  }
};


static
void grep_file(const std::string& tag, const char* path, Parameters& par) {
  par.search_tag = tag;
  par.path = path;
  if (std::strcmp(path, "-") == 0) {
    pegtl::cstream_input<> in(stdin, 16*1024, "stdin");
    pegtl::parse<rules::file, Search, cif::Errors>(in, par);
  } else if (gemmi::ends_with(path, ".gz")) {
    size_t orig_size = cif::estimate_uncompressed_size(path);
    std::unique_ptr<char[]> mem = cif::gunzip_to_memory(path, orig_size);
    pegtl::memory_input<> in(mem.get(), orig_size, path);
    pegtl::parse<rules::file, Search, cif::Errors>(in, par);
  } else {
    pegtl::file_input<> in(path);
    pegtl::parse<rules::file, Search, cif::Errors>(in, par);
  }
  fflush(stdout);
}

int main(int argc, char **argv) {
  if (argc < 1)
    return 2;
  option::Stats stats(usage, argc-1, argv+1);
  std::vector<option::Option> options(stats.options_max);
  std::vector<option::Option> buffer(stats.buffer_max);
  option::Parser parse(usage, argc-1, argv+1, options.data(), buffer.data());
  if (parse.error() || options[Unknown] ||
      (!options[Help] && parse.nonOptionsCount() < 2)) {
    option::printUsage(fwrite, stderr, usage);
    return 1;
  }
  if (options[Help]) {
    option::printUsage(fwrite, stdout, usage);
    return 0;
  }

  Parameters params;
  if (options[MaxCount])
    params.max_count = std::strtol(options[MaxCount].arg, nullptr, 10);
  if (options[WithFileName])
    params.with_filename = true;
  if (options[NoBlockName])
    params.with_blockname = false;
  if (options[WithTag])
    params.with_tag = true;
  if (options[Summarize])
    params.summarize = true;
  if (options[MatchingFiles])
    params.only_filenames = true;
  if (options[NonMatchingFiles]) {
    params.only_filenames = true;
    params.inverse = true;
  }
  if (options[Count])
    params.print_count = true;

  std::string tag = parse.nonOption(0);

  for (int i = 1; i < parse.nonOptionsCount(); ++i) {
    const char* path = parse.nonOption(i);
    try {
      grep_file(tag, path, params);
    } catch (std::runtime_error& e) {
      fprintf(stderr, "Error when parsing %s:\n\t%s\n", path, e.what());
      return 1;
    }
  }
  return 0;
}

// vim:sw=2:ts=2:et:path^=include,third_party
