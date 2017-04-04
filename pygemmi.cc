// Copyright 2017 Global Phasing Ltd.

#include "cif.hh"
#include "cifgz.hh"
#include "to_json.hh"
#include <sstream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

template<typename T>
std::string str_join(const T &iterable, const std::string& sep) {
  std::string r;
  bool first = true;
  for (const std::string& i : iterable) {
    if (!first)
      r += sep;
    r += i;
    first = false;
  }
  return r;
}

PYBIND11_PLUGIN(gemmi) {
  using namespace gemmi::cif;

  py::module mg("gemmi", "General MacroMolecular I/O");
  py::module cif = mg.def_submodule("cif", "CIF file format");

  py::class_<Document>(cif, "Document")
    .def(py::init<>())
    .def(py::init<const std::string &>())
    .def("__len__", [](const Document& d) { return d.blocks.size(); })
    .def("__iter__", [](const Document& d) {
        return py::make_iterator(d.blocks.begin(), d.blocks.end());
    }, py::keep_alive<0, 1>())
    .def("read_file", &Document::read_file,
         "Read file copying data into Document")
    .def("read_string", &Document::read_string,
         "Read a string as a CIF file")
    .def("sole_block", &Document::sole_block,
         "Returns the only block if there is exactly one")
    .def("as_json", [](const Document& d) {
        std::ostringstream os;
        JsonWriter(os).write_json(d);
        return os.str();
    });

  py::class_<Block>(cif, "Block")
    .def(py::init<>())
    .def_readonly("name", &Block::name)
    .def("find_value", &Block::find_value, py::return_value_policy::reference)
    .def("find_loop", &Block::find_loop)
    .def("find_loop_values", &Block::find_loop_values)
    .def("__repr__", [](const Block &self) {
        return "<gemmi.cif.Block " + self.name + ">";
    });

  py::class_<Loop>(cif, "Loop")
    .def(py::init<>())
    .def("width", &Loop::width)
    .def("length", &Loop::length)
    .def("__iter__", [](const Loop& self) {
        return py::make_iterator(self.tags.begin(), self.tags.end());
    }, py::keep_alive<0, 1>())
    .def("val", &Loop::val)
    .def("__repr__", [](const Loop &self) {
        return "<gemmi.cif.Loop " + std::to_string(self.length()) + "x" +
                                    std::to_string(self.width()) + ">";
    });

  py::class_<LoopColumn>(cif, "LoopColumn")
    .def(py::init<>())
    .def_readonly("loop", &LoopColumn::loop)
    .def_readwrite("col", &LoopColumn::col)
    .def("__iter__", [](const LoopColumn& self) {
        return py::make_iterator(self.begin(), self.end());
    }, py::keep_alive<0, 1>())
    .def("__bool__", [](const LoopColumn &self) -> bool { return self.loop; })
    .def("__repr__", [](const LoopColumn &self) {
        return "<gemmi.cif.LoopColumn " +
        (self.loop ? self.loop->tags[self.col].tag + " length " +
                     std::to_string(self.loop->length()) : "nil") + ">";
    });

  py::class_<LoopTable> lt(cif, "LoopTable");
  lt.def(py::init<>())
    .def_readonly("loop", &LoopTable::loop)
    .def_readonly("cols", &LoopTable::cols)
    .def("__iter__", [](const LoopTable& self) {
        return py::make_iterator(self.begin(), self.end());
    }, py::keep_alive<0, 1>())
    .def("__bool__", [](const LoopTable &self) -> bool { return self.loop; })
    .def("__repr__", [](const LoopTable &self) {
        return "<gemmi.cif.LoopTable " +
        (self.loop ? std::to_string(self.loop->length()) + "x" +
                     std::to_string(self.cols.size()) : "nil") + ">";
    });
  py::class_<LoopTable::Row>(lt, "Row")
    .def("raw", &LoopTable::Row::raw)
    .def("as_str", &LoopTable::Row::as_str)
    .def("as_num", &LoopTable::Row::as_num)
    .def("__repr__", [](const LoopTable::Row &self) {
        return "<gemmi.cif.LoopTable.Row: " + str_join(self, " ") + ">";
    });

  cif.def("read_any", &read_any, "Reads normal or gzipped cif file.");

  return mg.ptr();
}

// vim:sw=2:ts=2:et