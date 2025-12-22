SimpleParse Documentation
=========================

**A Parser Generator for Python**

SimpleParse is a BSD-licensed Python package providing a simple and fast parser
generator using a modified version of the mxTextTools text-tagging engine.
SimpleParse allows you to generate parsers directly from your EBNF grammar.

Unlike most parser generators, SimpleParse generates single-pass parsers (there
is no distinct tokenization stage), an approach taken from the predecessor
project (mcf.pars) which attempted to create "autonomously parsing regex objects".
The resulting parsers are not as generalized as those created by, for instance,
the Earley algorithm, but they do tend to be useful for the parsing of computer
file formats and similar structured text (as distinct from natural language parsing).

SimpleParse includes a patched copy of the mxTextTools tagging library with a
non-recursive rewrite of the core parsing loop. This provides a uniform parsing
platform where all features are always available.

SimpleParse is developed `on GitHub <https://github.com/mcfletch/simpleparse>`_
where you can create issues and pull requests.

Quick Start
-----------

.. code-block:: python

    from simpleparse.parser import Parser

    # Define a grammar
    grammar = '''
    root      := (word / space)+
    word      := [a-zA-Z]+
    space     := [ ]+
    '''

    # Create a parser
    parser = Parser(grammar, 'root')

    # Parse some text
    success, results, next_pos = parser.parse("hello world")
    print(f"Success: {success}, parsed {next_pos} characters")

Installation
------------

SimpleParse requires Python 3.3 or above. If you are compiling from source,
you'll also need a C compiler compatible with your Python. Pre-built wheels
are available for common platforms.

.. code-block:: bash

    pip install SimpleParse

For development installation:

.. code-block:: bash

    pip install -e ".[dev]"

Features & Changelog
--------------------

New in 3.0
~~~~~~~~~~

* **Multi-width Unicode engine** — Three implementations handle different string widths:

  * **UCS-1 (Latin-1)** — 1-byte characters, uses ``memchr`` for single-character searches
  * **UCS-2 (BMP)** — 2-byte characters for the Basic Multilingual Plane
  * **UCS-4** — 4-byte characters for the full Unicode range including astral planes
  * **Byte strings** — Direct parsing of ``bytes`` objects without decoding

  The engine selects the appropriate implementation based on the input string's
  internal representation.

* **Encoding parameter** — Parse UTF-8 or other encoded bytes with the
  ``encoding`` parameter; results are byte positions
* **Python 3.3+ only** — Python 2 compatibility code removed
* **Python 3.12, 3.13, 3.14 support** — Tested with latest Python versions
* **Modern build system** — Uses pyproject.toml and modern setuptools

New in 2.2.x
~~~~~~~~~~~~

* Python 3.8, 3.9, 3.10, 3.11 support
* Build process improvements
* Tox-based test suite

General Features
~~~~~~~~~~~~~~~~

* Simple-to-use interface — define an EBNF and start parsing
* Fast for small to medium files — this is primarily a feature of the
  underlying TextTools engine
* Allows pre-built and external parsing functions for tricky parsing tasks
* "Error on fail" error-reporting facility for parser syntax errors
* LookAhead mechanism for "is followed by" / "is not followed by" patterns
* Case-insensitive literals with ``c"literal"`` syntax
* Library of common constructs (simpleparse.common)
* Hexadecimal escapes for strings and character ranges

Parser Characteristics
~~~~~~~~~~~~~~~~~~~~~~

SimpleParse parsers are top-down, non-tokenizing parsers. They work from the
top of the parsing graph (the root production) and are closest to deterministic
recursive-descent parsers.

There are no backtracking facilities — any ambiguity is handled by choosing the
first successful match of a grammar (not the longest). As a result, the parsers
are entirely deterministic.

For simple deterministic grammars, parsing time should be close to linear with
the length of the text. SimpleParse parsers will generally be faster than
anything coded directly in Python, though they won't outperform grammar-specific
parsers written in C.

Parsing Encoded Bytes
---------------------

SimpleParse 3.0 introduces the ``encoding`` parameter for parsing byte strings
with Unicode grammars:

.. code-block:: python

    from simpleparse.parser import Parser

    # Define a grammar with Unicode characters
    grammar = '''
    root := (word / space)+
    word := [a-zA-Z\u4e00-\u9fff]+
    space := [ ]+
    '''

    p = Parser(grammar, 'root')

    # Parse UTF-8 encoded bytes
    text = 'hello 世界'.encode('utf-8')
    result = p.parse(text, encoding='utf-8')

    # result[2] is byte position (12), not character position (8)
    print(f"Parsed {result[2]} bytes")

**Supported encodings:**

* **UTF-8** — Full Unicode support with multi-byte sequences
* **Single-byte encodings** — Latin-1, ISO-8859-\*, Windows-1252, ASCII

.. note::

    When using the ``encoding`` parameter, all positions in the result are
    *byte positions*, not character positions. This allows direct slicing of
    the original byte string.

Documentation
-------------

.. toctree::
   :maxdepth: 2
   :caption: User Guide

   scanning
   grammars
   result_trees
   common_problems

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/parser
   api/common

.. toctree::
   :maxdepth: 1
   :caption: About

   license

Indices and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
