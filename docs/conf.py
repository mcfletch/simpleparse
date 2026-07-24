# Configuration file for the Sphinx documentation builder.

import os
import sys

# Add the project root to the path for autodoc
sys.path.insert(0, os.path.abspath('..'))

# -- Project information -----------------------------------------------------

project = 'SimpleParse'
copyright = '1998-2025, Mike C. Fletcher'
author = 'Mike C. Fletcher'

# The full version, including alpha/beta/rc tags
release = '3.0.0a3'
version = '3.0'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.intersphinx',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'display_version': True,
}

html_context = {
    'display_github': True,
    'github_user': 'mcfletch',
    'github_repo': 'simpleparse',
    'github_version': 'master',
    'conf_py_path': '/docs/',
}

# -- Options for autodoc -----------------------------------------------------

autodoc_default_options = {
    'members': True,
    'member-order': 'bysource',
    'special-members': '__init__',
    'undoc-members': True,
    'show-inheritance': True,
}

autodoc_mock_imports = ['simpleparse.stt.TextTools.mxTextTools.mxTextTools']

# -- Intersphinx configuration -----------------------------------------------

intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
}
