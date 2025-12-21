Common Problems
===============

Common errors, anti-patterns and known issues with the SimpleParse engine.

Repetition-as-Recursion
-----------------------

Using recursion for repetition is **extremely inefficient**. It generates 4 new
Python objects and a number of new object pointers for every match (figure >100
bytes for each match), on top of the engine overhead in tracking the recursion.
So if you have a 1-million character match that's "matching" for every character,
you'll have hundreds of megabytes of memory used.

.. code-block:: text

    # Bad - uses recursion for repetition
    a := 'b', a?

    # Good - uses repetition operator
    a := 'b'+

Null-match Children of Repeating Groups
---------------------------------------

At present, there's no way for the engine to know whether a child has been
satisfied (matched) because they are optional (or all of their children are
optional), or because they actually matched. The problem with the obvious
solution of just checking whether we've moved forward in the text is that many
classes of match object may match depending on external (non-text-based)
conditions, so if we do the check, all of those mechanisms suddenly fail.

For now, make sure:

* **No child** of a **repeating FirstOfGroup** ``(x/y/z)+`` or ``(x/y/z)*``
  can **match a Null-string**
* **At least one child** of a **repeating SequentialGroup** ``(x,y,z)+`` or
  ``(x,y,z)*`` **must not match** the Null-string

**Symptom:** The process goes into an endless loop with little or no memory
being consumed.

No Backtracking
---------------

The TextTools engine does not support backtracking as seen in regex engines
and many parsers, so productions like this can never match:

.. code-block:: text

    # This can never match - the FirstOfGroup consumes all 'c's
    a := (b/c)*, c

Because the 'c' productions will all have been consumed by the FirstOfGroup,
so the last 'c' can never match. This is a fundamental limit of the current
back-end, so unless a new back-end is created, the problem will not go away.
You will need to design your grammars accordingly.

First-Of, not Longest-Of
------------------------

The production ``c := (a/b)`` produces a FirstOfGroup — it **matches the first
child to match**, not the longest. Many parsers and regex engines use an
algorithm that matches all children and chooses the longest successful match.

It would be possible to define a new TextTools tagging command to support the
longest-of semantics for Table/SubTable matches, but this has not been
implemented. If such a command is created, it will likely be spelled ``|``
rather than ``/`` in the SimpleParse grammar.

Grouping Rules
--------------

The current grouping rule is that alternation binds closer than sequences,
so the grammar:

.. code-block:: text

    a,b,c/d,e

is interpreted as:

.. code-block:: text

    a,b,(c/d),e

Note that this is different from:

.. code-block:: text

    (a,b,c)/(d,e)  # This is NOT how it works!

When in doubt, use explicit parentheses to make the grouping clear.

Encoding and Byte Position Issues
---------------------------------

When using the ``encoding`` parameter to parse encoded byte strings, remember
that all positions in the result tree are *byte positions*, not character
positions. This is particularly important with multi-byte encodings like UTF-8:

.. code-block:: python

    # UTF-8 example: "hello 世界" is 12 bytes but 8 characters
    text = 'hello 世界'.encode('utf-8')
    result = parser.parse(text, encoding='utf-8')

    # result[2] will be 12 (bytes), not 8 (characters)
    success, trees, byte_position = result

If you need character positions, you'll need to convert them yourself using
the encoding information.

Unicode Character Ranges
------------------------

SimpleParse 3.0 supports full Unicode character ranges in grammars. You can
use Unicode escape sequences in character ranges:

.. code-block:: text

    # Match CJK characters
    cjk := [\u4e00-\u9fff]+

    # Match emoji (requires 8-digit escape for astral plane)
    emoji := [\U0001F600-\U0001F64F]+

Note that the number of hex digits is fixed: 4 for ``\u`` and 8 for ``\U``.
