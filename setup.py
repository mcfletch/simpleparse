#!/usr/bin/env python
"""Installs SimpleParse using distutils

Run:
	python setup.py install
to install the packages from the source archive.
"""
from setuptools import setup, Extension
import os, sys, string

def findVersion( ):
	a = {}
	exec( open( '__init__.py' ).read(), a, a )
	return a['__version__']

def isPackage( filename ):
	"""Is the given filename a Python package"""
	return (
		os.path.isdir(filename) and 
		os.path.isfile( os.path.join(filename,'__init__.py'))
	)
def packagesFor( filename, basePackage="" ):
	"""Find all packages in filename"""
	set = {}
	for item in os.listdir(filename):
		dir = os.path.join(filename, item)
		if string.lower(item) != 'cvs' and isPackage( dir ):
			if basePackage:
				moduleName = basePackage+'.'+item
			else:
				moduleName = item
			set[ moduleName] = dir
			set.update( packagesFor( dir, moduleName))
	return set

packages = packagesFor( ".", 'simpleparse' )
packages.update( {'simpleparse':'.'} )

options = {
	'sdist': { 'force_manifest':1,'formats':['gztar','zip'] },
}
if sys.platform == 'win32':
	options.setdefault(
		'build_ext',{}
	)['define'] = 'BAD_STATIC_FORWARD'

if __name__ == "__main__":
	from sys import hexversion
	if hexversion >= 0x2030000:
		# work around distutils complaints under Python 2.2.x
		extraArguments = {
			'classifiers': [
				"""Programming Language :: Python""",
				"""Topic :: Software Development :: Libraries :: Python Modules""",
				"""Intended Audience :: Developers""",
			],
			'keywords': 'parse,parser,parsing,text,ebnf,grammar,generator',
			'long_description' : """A Parser Generator for Python (w/mxTextTools derivative)

Provides a moderately fast parser generator for use with Python,
includes a forked version of the mxTextTools text-processing library
modified to eliminate recursive operation and fix a number of 
undesirable behaviours.

Converts EBNF grammars directly to single-pass parsers for many
largely deterministic grammars.""",
			'platforms': ['Any'],
		}
	else:
		extraArguments = {
		}
	setup (
		name = "SimpleParse",
		version = findVersion(),
		description = "A Parser Generator for Python (w/mxTextTools derivative)",
		author = "Mike C. Fletcher",
		author_email = "mcfletch@users.sourceforge.net",
		url = "http://simpleparse.sourceforge.net/",

		package_dir = packages,
		options = options,

		packages = packages.keys(),
		include_package_data = True,
		zip_safe = False,
		ext_modules=[
			Extension(
				"simpleparse.stt.TextTools.mxTextTools.mxTextTools", 
				[
					'stt/TextTools/mxTextTools/mxTextTools.c',
					'stt/TextTools/mxTextTools/mxte.c',
					'stt/TextTools/mxTextTools/mxbmse.c',
				],
				include_dirs=['stt/TextTools/mxTextTools'],
				define_macros=[ ('MX_BUILDING_MXTEXTTOOLS',1) ],
			),
		],
		**extraArguments
	)
