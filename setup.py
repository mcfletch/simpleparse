#!/usr/bin/env python
"""Installs SimpleParse using distutils

Run:
    python setup.py install
to install the packages from the source archive.
"""
from setuptools import setup, Extension, find_packages
import os, sys
HERE = os.path.abspath(os.path.dirname(__file__))

# Determine Python version for conditional compilation
PY_MAJOR = sys.version_info.major
PY_MINOR = sys.version_info.minor
PYTHON_VERSION = (PY_MAJOR, PY_MINOR)

def findVersion( ):
    """Find the version declaration in the __init__.py file"""
    for line in open( 
        os.path.join( HERE, 'simpleparse', '__init__.py') 
    ).read().splitlines():
        if line.startswith('__version__'):
            line = line.split('=')[1]
            line = line.strip().strip('"').strip("'")
            return line
    raise RuntimeError("Unable to find __version__ declaration")

options = {
    'sdist': { 'force_manifest':1,'formats':['gztar'] },
}
if sys.platform == 'win32':
    options.setdefault(
        'build_ext',{}
    )['define'] = 'BAD_STATIC_FORWARD'

if __name__ == "__main__":
    # Most metadata is now in pyproject.toml
    # This setup.py is primarily for C extension configuration
    
    # Python 3.3+ only - use modern implementation with compatibility layer
    print(f"Building for Python {PY_MAJOR}.{PY_MINOR}: Modern Unicode implementation")
    
    # Verify minimum Python version
    if PYTHON_VERSION < (3, 3):
        raise RuntimeError(f"SimpleParse requires Python 3.3 or later, got {PY_MAJOR}.{PY_MINOR}")
    
    # Use modern implementation with compatibility shims for deprecated APIs
    sources = [
        'simpleparse/stt/TextTools/mxTextTools/mxTextTools.c',
        'simpleparse/stt/TextTools/mxTextTools/mxte_modern.c',
        'simpleparse/stt/TextTools/mxTextTools/mxte_smart.c', 
        'simpleparse/stt/TextTools/mxTextTools/mxbmse.c',
    ]
    
    define_macros = [ 
        ('MX_BUILDING_MXTEXTTOOLS', 1),
        ('PY_SSIZE_T_CLEAN', 1),
        ('DEBUG', 1),
    ]
    
    # For all Python 3.3+, force use of modern APIs and eliminate legacy compatibility
    define_macros.append(('MODERN_UNICODE_ONLY', 1))  # Enable modern Unicode-only build
    define_macros.append(('FORCE_MODERN_UNICODE', 1))  # Force use of modern APIs
    
    setup(
        ext_modules=[
            Extension(
                "simpleparse.stt.TextTools.mxTextTools.mxTextTools", 
                sources,
                include_dirs=[
                    'simpleparse/stt/TextTools/mxTextTools',
                ],
                define_macros=define_macros,
            ),
        ],
        options=options,
    )
