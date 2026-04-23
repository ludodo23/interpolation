## docs/conf.py — Sphinx configuration for interpolation

import subprocess, os

# -- Project info -------------------------------------------------------------
project   = "interpolation"
copyright = "2026, Ludovic Andrieux"
author    = "Ludovic Andrieux"
release   = "0.2.0"

# -- Extensions ---------------------------------------------------------------
extensions = [
    "breathe",          # bridge Doxygen XML → Sphinx
    "myst_parser",      # parse .md files
    "sphinx.ext.autodoc",
    "sphinx.ext.viewcode",
    "sphinx_rtd_theme",
]

# -- MyST (Markdown) ----------------------------------------------------------
myst_enable_extensions = ["colon_fence", "deflist"]
source_suffix = {".rst": "restructuredtext", ".md": "markdown"}

# -- Breathe ------------------------------------------------------------------
breathe_projects        = {"interpolation": "../doxygen/xml"}
breathe_default_project = "interpolation"

# -- HTML output --------------------------------------------------------------
html_theme = "sphinx_rtd_theme"
html_theme_options = {
    "navigation_depth": 4,
    "collapse_navigation": False,
    "titles_only": False,
}
html_static_path = ["_static"]

# -- General ------------------------------------------------------------------
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]
