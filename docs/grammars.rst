SimpleParse Grammars
====================

SimpleParse uses a particular EBNF grammar which reflects the current set of
features in the system. Though the system is modular enough that you could
replace that grammar, most users will simply want to use the provided grammar.
This document provides a quick reference for the various features of the grammar
with examples of use and descriptions of their effects.

**Prerequisites:**

* Python 3.x programming
* Some familiarity with EBNF grammars and other parsing terminology

An Example
----------

Here is an example of a basic SimpleParse grammar:

.. code-block:: python

    declaration = r'''# note use of raw string when embedding in python code...
    file           :=  [ \t\n]*, section+
    section        :=  '[',identifier!,']'!, ts,'\n', body
    body           :=  statement*
    statement      :=  (ts,semicolon_comment)/equality/nullline
    nullline       :=  ts,'\n'
    comment        :=  -'\n'*
    equality       :=  ts, identifier,ts,'=',ts,identified,ts,'\n'
    identifier     :=  [a-zA-Z], [a-zA-Z0-9_]*
    identified     :=  string/number/identifier
    ts             :=  [ \t]*
    '''

You can see that the format allows for comments in Python style, and fairly
free-form treatment of whitespace around the various items (i.e. ``s:=x`` and
``s := x`` are equivalent). The grammar is actually written such that you can
break productions (rules) across multiple lines if that will make your grammar
more readable. The grammar also allows both ``:=`` and ``::=`` for the
"defined as" symbol.

Element Tokens
--------------

Element tokens are the basic operational unit of the grammar. The concrete
implementation of the various tokens is the module ``simpleparse.objectgenerator``,
their syntax is defined in the module ``simpleparse.simpleparsegrammar``. You can
read a formal definition of the grammar used to define them at the end of this
document.

Character Range
~~~~~~~~~~~~~~~

::

    [ \t\n]
    [a-zA-Z]
    [a-zA-Z0-9_]

Matches any 1 character in the given range.

String Literal
~~~~~~~~~~~~~~

::

    "["
    '['
    '\t'
    '\xa0'

Match the sequence of characters as given, allowing for special, octal and
hexadecimal escape characters.

Case-insensitive String Literal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    c"this"
    c'this'
    c' this\t\n'
    c'\xa0'

Match the sequence of characters without regard to the case of the target text,
allowing for special, octal and hexadecimal escape characters.

.. note::

    Case-insensitive literals are slower than regular literals.

Name Reference
~~~~~~~~~~~~~~

::

    statement
    semicolon_comment
    ts

Match the production whose name is specified. Those productions may have been
included from a library module or created by you and passed to the Parser
object's initialiser.

Sequential Groups
~~~~~~~~~~~~~~~~~

::

    (a,b,c,d)

Match a sequence of element token children. Sequential groups have a lower
precedence than FirstOf groups (below), so the group ``(a,b/c,d)`` is equivalent
to ``(a,(b/c),d)``.

FirstOf Groups
~~~~~~~~~~~~~~

::

    (a/b/c/d)

Match the first child which matches.

.. note::

    This is very different from systems which parse all children and choose the
    longest/most successful child-match.

Sequential groups have a lower precedence than FirstOf groups, so the group
``(a,b/c,d)`` is equivalent to ``(a,(b/c),d)``.

Error On Fail (Cut)
~~~~~~~~~~~~~~~~~~~

::

    !
    ! "Error Message"

    (a,!,b,c,d,e,f)

Used as a "token", the ErrorOnFail modifier (also called "cut" after Prolog's
cut directive), declares that all subsequent items in the enclosing sequential
group should be marked ErrorOnFail, as if the given ErrorOnFail modifier were
applied to each one individually.

.. note::

    Can only be used as a member of a Sequential group, cannot be a member of a
    FirstOf group.

See the section :ref:`modifiers` below for more details of the semantics
surrounding this token.

Character Classes, Strings and Escape Characters
-------------------------------------------------

Both character classes and strings in SimpleParse may use octal escaping (of 1
to 3 octal digits), hexadecimal escaping (2 digits), Unicode escaping (4 or 8
digits), or standard Python character escapes (``\a\b\f\n\r\t\v``).

* ``\n`` is interpreted according to the local machine, and thus may be
  non-portable, so an explicit declaration using hexadecimal code might be more
  suitable in cases where differentiation is required.
* The number of digits is fixed for hexadecimal and Unicode escaping.
* You can use ``\u`` or ``\U`` escape codes to define Unicode characters in
  your grammars.

Strings may be either single or double quoted (but not triple quoted).

To include a ``]`` character in a character class, make it the first character
of the class. Similarly, a literal ``-`` character must be either the first
(after the optional ``]`` character) or the last character. The grammar
definition for a character class is as follows::

    '[', CHARBRACE?,CHARDASH?, (CHARRANGE/CHARNOBRACE)*, CHARDASH?, ']'

It is a common error to have declared something like ``[+-*]`` as a character
range (every character including and between + and \*) intending to specify
``[-+*]`` or ``[+*-]`` (three distinct characters). Symptoms include not
matching '-' or matching characters that were not expected.

.. _modifiers:

Modifiers/Operators
-------------------

Each element token can have a prefix and/or a postfix modifier applied to it
to alter how the engine treats a match of the "base" element token.

Negation Modifier (-)
~~~~~~~~~~~~~~~~~~~~~

::

    -"this"
    -those*
    -[A-Z]+
    -(them,their)
    -(them/their)

Match a single character at the current position if the entire base element
token doesn't match. If repeating, match any number of characters until the
base element token matches.

Optional Modifier (? postfix)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    "this"?
    those?
    [A-Z]?
    (them,their)?
    (them/their)?

Match the base element token, or if the base element token cannot match,
match nothing.

LookAhead Modifier (? prefix)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    ?"this"
    ?-those
    ?[A-Z]
    ?-(them,their)

Match the base element token, then return to the previous position (this is
called "LookAhead" in the mx.TextTools documentation). The ``-`` modifier is
applied "after" the lookahead, so that a lookahead on a negative match equates
to "is not followed by", while lookahead on positive matches equates to
"is followed by".

Repetition Modifiers (\* and +)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    "this"*
    those*
    [A-Z]*
    (them,their)*

Match the base element token from 0 to an infinite number of times (``*``), or
from 1 to an infinite number of times (``+``).

Error On Fail Modifier (!)
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    "this"!
    "this"+!

    "those" ! "Expected 'those' at position %(position)s"

Consider a failure to match a SyntaxError (stop parsing, and raise an exception).
If the optional string-literal is included, it specifies the message (template)
to be used for the SyntaxError. You can use ``%(varname)s`` formats to have the
following variables substituted:

* **position** — the character position at which parsing failed
* **line** — the (approximate) line on which parsing failed
* **lineChar** — the position on the line where parsing failed
* **expected** — the grammar's definition of the failed item
* **production** — the top-level production which included the failing item
* **text** — the text from the failure position to 50 characters beyond

.. note::

    The error_on_failure flag is ignored for optional items (since they can
    never fail), and only raises an error if a repeating non-optional production
    fails completely.

Using the ErrorOnFail Operator
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using the ErrorOnFail operator can be somewhat tricky. It is often easier to
use the "stand-alone" element-token version of cut. Here's an example of use:

.. code-block:: text

    top := a/b/bp
    a   := 'a', !, ws, '=', ws, something
    b   := 'b', ws, '=', !, ws, something
    bp  := 'b', ws, '+=', !, ws, something

The production ``top`` can match an 'a =', a 'b =', or a 'b +=', but if it
encounters an 'a' without an '=' following, it will raise a syntax error. For
the two "b" productions, we don't want to raise a Syntax error if the 'b' is
not followed by the '=' or '+=' because the grammar might match the other
production, so we only cut off back-tracking after the operator is found.

Consider this alternative:

.. code-block:: text

    top := a/b/bp
    a   := 'a'!, ws, '=', ws, something # BAD DON'T DO THIS!
    b   := 'b', ws, '=', !, ws, something
    bp  := 'b', ws, '+=', !, ws, something

This grammar does something very different (and somewhat useless). When the
"top" production goes to match, it tries to match the "a" production, which
tries to match the 'a' literal. If literal isn't there, for instance, for the
text 'b =', then the 'a' literal will raise a SyntaxError. The result is that
the "b" and "bp" productions can **never** match with this grammar.

Declarations (Productions) and Result Trees
-------------------------------------------

Each SimpleParse grammar is a series of declarations which define a production
(rule) and bind it to a name which can be referenced by any production in the
declaration set. Defining a production generally causes a result tuple to be
created in the results tree (see below for what else can happen).

Default Result Tree Generation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Here are some examples showing sample productions and the result trees they
would generate:

**Production:** ``s := "this"``

**Result:**

.. code-block:: python

    ('s', start, stop, [])  # no children

**Production:** ``s := them, those?``

**Result:**

.. code-block:: python

    ('s', start, stop, [
        ("them", start, stop, [...]),
        ("those", start, stop, [...])
    ])
    # or if optional not present:
    ('s', start, stop, [
        ("them", start, stop, [...])
    ])

**Production:** ``s := them*, those``

**Result:**

.. code-block:: python

    ('s', start, stop, [
        ("them", start, stop, [...]),
        ("them", start, stop, [...]),
        ("those", start, stop, [...])
    ])
    # or if optional repeating not present:
    ('s', start, stop, [
        ("those", start, stop, [...])
    ])

As a general rule, when a production matches, a match tuple is added to the
result tree. The first item of this tuple is normally the name of the production
(as a string), the second is the starting position of the match, the third is
the stopping position of the match, and the fourth is a list of any
child-production's result trees.

Expanded and Unreported Productions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using these features allows you to trim unwanted entries out of your results
tree (which is good for efficiency, as the system doesn't need to store the
result-trees). Using expanded productions can allow you to reduce the complexity
of your grammars by factoring out common patterns and allowing them to be
included in multiple productions without generating extraneous result-tuples
in the results tree.

**Normal Production:**

.. code-block:: text

    a := (b,c)

Result:

.. code-block:: python

    ('a', l, r, [
        ("b", l, r, [...]),
        ("c", l, r, [...])
    ])

**Unreported Production:**

.. code-block:: text

    a := (b,c)
    # b is "unreported"
    <b> := 'b'

Result:

.. code-block:: python

    ('a', l, r, [
        # no b, no children of b
        ("c", l, r, [...])
    ])

**Expanded Production:**

.. code-block:: text

    a := (b,c)
    # b is "expanded"
    >b< := (d,e)

Result:

.. code-block:: python

    ('a', l, r, [
        # children of b are returned as if they
        # were direct children of a
        ("d", l, r, [...]),
        ("e", l, r, [...]),
        ("c", l, r, [...])
    ])

Both of these methods still produce standard results trees so no special work
is required to process the results tree. (There are methods described in
:doc:`result_trees` which can generate non-standard result trees for special
purposes).

Pre-built and Library Element Tokens
------------------------------------

Sometimes the parsing library can't handle a particular matching task, or a
method is easier than an EBNF grammar. Common patterns like floating point
numbers and escaped strings are provided in a parsing library.

SimpleParse allows you to pass a set of "pre-built" element tokens to the
Parser during initialization. These pre-built parsers can either be instances
of ``simpleparse.objectgenerator.ElementToken``, or raw mx.TextTools tag-tables.
To use them, pass the Parser's initializer a list of two-tuples of
(name, parserObject):

.. code-block:: python

    parser = Parser(declaration, "v", prebuilts=[
        ("word", parserObject1),
        ("white", parserObject2),
    ])

You can see a working example (which uses Python's ``re`` module to create a
prebuilt parser) in ``examples/prebuilt_call.py``.

SimpleParse provides the ability to create libraries of common parsers for
inclusion in other parsers. The ``simpleparse.common`` package includes numbers,
basic strings, the ISO date format, some character types, and some comment types.
New contributions to the library are welcome.

In general, importing a particular module from the common package makes the
production names from the module available in any subsequent grammar defined.
Refer to the documentation for a particular module to see what production names
are exported.

.. code-block:: python

    from simpleparse.common import strings, comments, numbers

Many of the standard common parser modules also include "Interpreter" objects
which can be used to process the results tree generated by the mini-grammar
into a Python-friendly form. See the documentation for the individual modules.

.. code-block:: python

    class MyParser(Parser):
        string = strings.StringInterpreter()

Formal SimpleParse EBNF Grammar
-------------------------------

This is the formal definition of the SimpleParse grammar. Although the grammar
is functional (should parse any proper grammar), the grammar used during parser
generation is a manually generated version found in the
``simpleparse.simpleparsegrammar`` module.

.. code-block:: text

    declarationset      :=  declaration+
    declaration         :=  ts, (unreportedname/expandedname/name), ts, ':', ':'?, '=', seq_group

    element_token       :=  lookahead_indicator?, ts, negpos_indicator?, ts,
                            (literal/range/group/name), ts, occurence_indicator?, ts,
                            error_on_fail?

    negpos_indicator    :=  [-+]
    lookahead_indicator :=  "?"
    occurence_indicator :=  [+*?]
    error_on_fail       :=  "!", (ts, literal)?

    >group<             :=  '(', seq_group, ')'
    seq_group           :=  ts, (error_on_fail/fo_group/element_token),
                            (ts, seq_indicator, ts,
                                (error_on_fail/fo_group/element_token)
                            )*, ts

    fo_group            :=  element_token, (ts, fo_indicator, ts, element_token)+

    # following two are likely something people might want to
    # replace in many instances...
    <fo_indicator>      :=  "/"
    <seq_indicator>     :=  ','

    unreportedname      :=  '<', name, '>'
    expandedname        :=  '>', name, '<'
    name                :=  [a-zA-Z_], [a-zA-Z0-9_]*
    <ts>                :=  ([ \011-\015]+ / comment)*
    comment             :=  '#', -'\n'*, '\n'
    literal             :=  literalDecorator?, ("'", (CHARNOSNGLQUOTE/ESCAPEDCHAR)*, "'") /
                            ('"', (CHARNODBLQUOTE/ESCAPEDCHAR)*, '"')
    literalDecorator    :=  [c]

    range               :=  '[', CHARBRACE?, CHARDASH?, (CHARRANGE/CHARNOBRACE)*, CHARDASH?, ']'
    CHARBRACE           :=  ']'
    CHARDASH            :=  '-'
    CHARRANGE           :=  CHARNOBRACE, '-', CHARNOBRACE
    CHARNOBRACE         :=  ESCAPEDCHAR/CHAR
    CHAR                :=  -[]]
    ESCAPEDCHAR         :=  '\\', (SPECIALESCAPEDCHAR / ('x', HEXESCAPEDCHAR) /
                            ("u", UNICODEESCAPEDCHAR_16) / ("U", UNICODEESCAPEDCHAR_32) /
                            OCTALESCAPEDCHAR)
    SPECIALESCAPEDCHAR  :=  [\\abfnrtv"']
    OCTALESCAPEDCHAR    :=  [0-7], [0-7]?, [0-7]?
    HEXESCAPEDCHAR      :=  [0-9a-fA-F], [0-9a-fA-F]
    CHARNODBLQUOTE      :=  -[\\"]+
    CHARNOSNGLQUOTE     :=  -[\\']+
    UNICODEESCAPEDCHAR_16 := [0-9a-fA-F], [0-9a-fA-F], [0-9a-fA-F], [0-9a-fA-F]
    UNICODEESCAPEDCHAR_32 := [0-9a-fA-F], [0-9a-fA-F], [0-9a-fA-F], [0-9a-fA-F],
                             [0-9a-fA-F], [0-9a-fA-F], [0-9a-fA-F], [0-9a-fA-F]
