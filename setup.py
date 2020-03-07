#!/usr/bin/env python
"""Installs SimpleParse using distutils

Run:
    python setup.py install
to install the packages from the source archive.
"""
from setuptools import setup, Extension, find_packages
import os, sys
HERE = os.path.abspath(os.path.dirname(__file__))

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
    'sdist': { 'force_manifest':1,'formats':['gztar','zip'] },
}
if sys.platform == 'win32':
    options.setdefault(
        'build_ext',{}
    )['define'] = 'BAD_STATIC_FORWARD'

if __name__ == "__main__":
    packages = find_packages(HERE)
    setup (
        name = "SimpleParse",
        version = findVersion(),
        description = "A Parser Generator for Python (w/mxTextTools derivative)",
        author = "Mike C. Fletcher",
        author_email = "mcfletch@users.sourceforge.net",
        url = "http://simpleparse.sourceforge.net/",

        options = options,

        packages = packages,
        ext_modules=[
            Extension(
                "simpleparse.stt.TextTools.mxTextTools.mxTextTools", 
                [
                    'simpleparse/stt/TextTools/mxTextTools/mxTextTools.c',
                    'simpleparse/stt/TextTools/mxTextTools/mxte.c',
                    'simpleparse/stt/TextTools/mxTextTools/mxbmse.c',
                ],
                include_dirs=[
                    'simpleparse/stt/TextTools/mxTextTools',
                ],
                define_macros=[ 
                    ('MX_BUILDING_MXTEXTTOOLS',1),
                    ('PY_SSIZE_T_CLEAN',1),
                    ('DEBUG',1),
                ],
            ),
        ],
        classifiers= [
            """Programming Language :: Python""",
            """Topic :: Software Development :: Libraries :: Python Modules""",
            """Intended Audience :: Developers""",
        ],
        keywords= 'parse,parser,parsing,text,ebnf,grammar,generator',
        long_description="""A Parser Generator for Python (w/mxTextTools derivative)

Provides a moderately fast parser generator for use with Python,
includes a forked version of the mxTextTools text-processing library
modified to eliminate recursive operation and fix a number of 
undesirable behaviours.

Converts EBNF grammars directly to single-pass parsers for many
largely deterministic grammars.""",
        platforms= ['Any'],
        include_package_data=True,
    )
