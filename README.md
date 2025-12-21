# SimpleParse 3.x
## A Parser Generator for Python and mxTextTools

SimpleParse is a parser generator, it takes a modified EBNF
grammar and customises a text-processing engine `mxTextTools`
to produce a relatively fast table-driven parser.

As of version 3.x, Simpleparse has moved to being unicode-native,
with direct support for each of the 3 internal unicode formats
that are supported by Python3 (as well as bytes format). However,
the effect of the unicode changes is that the parser generator will
assume unicode when generating the parser, and you will need to
specify the content encoding when parsing/tagging.

So for parsing a latin-1 document provided as raw encoded bytes:

* tag(bytestream, tagtable, 'latin-1')

Only utf-8 encoding is handled among multi-byte encodings, but
single-byte encodings should be handled. Note that it is *generally*
going to be more useful to load your document as unicode text and
then tag the content, but the byte-stream parsing may be necessary
if you have the content already in memory as bytes and can't 
decode it for some reason.

You can see a full-featured [sample parser](https://github.com/mcfletch/pyvrml97/blob/master/vrml/vrml97/parser.py)
in the `PyVRML97` package.

See the [docs](https://mcfletch.github.io/simpleparse/) for usage.

```pip install SimpleParse```

[![Appveyor Build](https://ci.appveyor.com/api/projects/status/MikeCFletcher/simpleparse?svg=True)](https://ci.appveyor.com/project/MikeCFletcher/simpleparse)

[![PyPI download month](https://img.shields.io/pypi/v/simpleparse.svg)](https://pypi.python.org/pypi/simpleparse)

[![PyPI download month](https://img.shields.io/pypi/dm/simpleparse.svg)](https://pypi.python.org/pypi/simpleparse/)

