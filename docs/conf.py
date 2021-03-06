# -*- coding: utf-8 -*-

# -- General configuration ------------------------------------------------

needs_sphinx = '1.4'

extensions = ['sphinx.ext.doctest', 'sphinx.ext.githubpages']

#templates_path = ['_templates']

source_suffix = '.rst'

master_doc = 'index'

project = u'Gemmi'
copyright = u'2017 Global Phasing Ltd'
author = u'Marcin Wojdyr'

version = u'0.0'
release = version

exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
pygments_style = 'sphinx'
todo_include_todos = False
highlight_language = 'c++'


# -- Options for HTML output ----------------------------------------------

import os
if not os.environ.get('READTHEDOCS'):
    #import guzzle_sphinx_theme
    #html_theme_path = guzzle_sphinx_theme.html_theme_path()
    #html_theme = 'guzzle_sphinx_theme'
    html_theme = 'sphinx_rtd_theme'
    # html_theme_options = {}

#html_static_path = ['_static']


# -- Options for LaTeX output ---------------------------------------------

latex_elements = {
    # 'papersize': 'letterpaper',
    # 'pointsize': '10pt',
    # 'preamble': '',
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'Gemmi.tex', u'Gemmi Documentation',
     u'Marcin Wojdyr', 'manual'),
]

