Scanning with SimpleParse
=========================

SimpleParse provides a parser generator which converts an EBNF grammar into a
run-time parser for use in scanning/marking up texts. This document describes
the process of developing and using an EBNF grammar to perform the text-scanning
process.

**Prerequisites:**

* Python 3.x programming
* Some familiarity with EBNF grammars and parsing terminology

Creation of a Simple Grammar
----------------------------

The primary function of SimpleParse is to convert an EBNF grammar into an
in-memory object which can do the work of scanning (and potentially processing)
data which conforms to that grammar.

For our first experiment, we'll define a simple grammar for use in parsing an
INI-file-like format:

.. code-block:: python

    from simpleparse.common import numbers, strings, comments

    declaration = r'''# note use of raw string when embedding in python code...
    file           :=  [ \t\n]*, section+
    section        :=  '[',identifier!,']'!, ts,'\n', body
    body           :=  statement*
    statement      :=  (ts,semicolon_comment)/equality/nullline
    nullline       :=  ts,'\n'
    equality       :=  ts, identifier,ts,'=',ts,identified,ts,'\n'
    identifier     :=  [a-zA-Z], [a-zA-Z0-9_]*
    identified     :=  string/number/identifier
    ts             :=  [ \t]*
    '''

The first line incorporates the ability to automatically include libraries of
commonly used productions. By importing these three modules, I've made the
productions "string", "number" and "semicolon_comment" (among others) available
to all the Parser instances I create.

**Error-on-fail Feature:** The ``identifier!`` and ``']'!`` element tokens in the
"section" production tell the parser generator to report a ParserSyntaxError if
we attempt to parse these element tokens and fail. We could also spell this:

.. code-block:: text

    section        :=  '[',!,identifier,']', ts,'\n', body

which is often easier to use in complex grammars.

Checking a Grammar
------------------

SimpleParse does not have a separate compilation step, but it's useful to set
up tests both for whether the grammar itself is syntactically correct, and for
whether the productions match the values you expect.

To check that a grammar is syntactically correct, attempt to create a Parser
with the grammar:

.. code-block:: python

    from simpleparse.parser import Parser
    parser = Parser(declaration)

If your grammar has syntax errors, you'll get a ValueError:

.. code-block:: text

    ValueError: Unable to complete parsing of the EBNF, stopped at line 3
    Unparsed:
    ts,'\n', body
    body           :=  statement*
    ...

Checking a Production
---------------------

Now that we have our Parser object, we can test that our productions match the
values we expect:

.. code-block:: python

    testEquality = [
        "s=3\n",
        "s = 3\n",
        '''  s="three\\nthere"\n''',
        '''  s=three\n''',
    ]

    production = "equality"

    for testData in testEquality:
        success, children, nextcharacter = parser.parse(testData, production=production)
        assert success and nextcharacter == len(testData), \
            f"Wasn't able to parse {testData!r} as {production}"

Scanning Text with the Grammar
------------------------------

In normal use, you'll want to define a default root production for the parser:

.. code-block:: python

    parser = Parser(declaration, "file")
    result = parser.parse(testData)

.. note::

    The root is treated differently than other productions — it doesn't return
    a result-tuple in the results tree, but instead governs the overall operation
    of the parser, determining whether it "succeeds" or "fails" as a whole.

You can read about how to process the results tree in :doc:`result_trees`.

Parsing Encoded Bytes
---------------------

SimpleParse 3.0 introduces the ``encoding`` parameter for parsing byte strings
with Unicode grammars:

.. code-block:: python

    # Parse UTF-8 encoded content
    with open('myfile.txt', 'rb') as f:
        content = f.read()

    result = parser.parse(content, encoding='utf-8')
    # result[2] is byte position, not character position

Supported encodings include UTF-8, Latin-1, ISO-8859-\*, Windows-1252, and ASCII.
