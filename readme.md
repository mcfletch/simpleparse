# SimpleParse 2.x
## A Parser Generator for Python and mxTextTools

SimpleParse is a parser generator, it takes a modified EBNF
grammar and customises a text-processing engine `mxTextTools`
to produce a relatively fast table-driven parser.

Development has moved to github, and currently all tests are passing
on Python 2.7, with 2 test failures each on the XML processing
tests for xml comments and prologues under python 3.6 and 3.7.

You can see a full-featured [sample parser](https://github.com/mcfletch/pyvrml97/blob/master/vrml/vrml97/parser.py)
in the `PyVRML97` package.

See the [docs](http://mcfletch.github.io/simpleparse) for usage.

```pip install SimpleParse```

[![Appveyor Build](https://ci.appveyor.com/api/projects/status/MikeCFletcher/simpleparse?svg=True)](https://ci.appveyor.com/project/MikeCFletcher/simpleparse)

[![PyPI download month](https://img.shields.io/pypi/v/simpleparse.svg)](https://pypi.python.org/pypi/simpleparse)

[![PyPI download month](https://img.shields.io/pypi/dm/simpleparse.svg)](https://pypi.python.org/pypi/simpleparse/)

