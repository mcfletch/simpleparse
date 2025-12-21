Processing Result Trees
=======================

SimpleParse parsers generate tree structures describing the structure of your
parsed content. This document briefly describes the structures, a simple
mechanism for processing the structures, and ways to alter the structures
as they are generated to accomplish specific goals.

**Prerequisites:**

* Python 3.x programming
* Familiarity with creating SimpleParse Parsers (see: :doc:`scanning`)

Standard Result Trees
---------------------

SimpleParse uses the same result format as is used for the underlying
mx.TextTools engine. The engine returns a three-item tuple from the parsing
of the top-level (root) production like so:

.. code-block:: python

    success, resultTrees, nextCharacter = myParser.parse(someText, processor=None)

* **success** — Boolean value indicating whether the production (by default the
  root production) matched (was satisfied) at all
* **resultTrees** — List of result tuples for the parsed content
* **nextCharacter** — Integer value indicating the next character to be parsed
  in the text (i.e. ``someText[startCharacter:nextCharacter]`` was parsed)

.. note::

    If success is false, then nextCharacter is set to the (very ill-defined)
    "error position", which is the position reached by the last TextTools
    command in the top-level production before the entire table failed. This
    is a lower-level value than is usefully predictable within SimpleParse
    (for instance, negative results which cause a failure will actually report
    the position after the positive version of the element token succeeds).
    You might use it as a hint to your users of where the error occurred, but
    using error-on-fail SyntaxErrors is **by far** the preferred method.
    Basically, if success is false, consider nextCharacter to contain garbage
    data.

When the processor argument to parse is false (or a non-callable object), the
system does not attempt to use the default processing mechanism, and returns
the result trees directly. The standard format for result-tree nodes is as
follows:

.. code-block:: python

    (production_name, start, stop, children_trees)

Where ``start`` and ``stop`` represent indexes in the source text such that
``sourcetext[start:stop]`` is the text which matched this production. The
**list of children** is the list of the result-trees for the child productions
within the production, or **None**.

.. warning::

    The ``children_trees`` may be ``None``, so you can't automatically do a
    "for" loop over the children_trees without checking first.

Expanded productions, as well as unreported productions (and the children of
unreported productions), will not appear in the result trees, neither will the
root production. See :doc:`grammars` for details. However, LookAhead productions
where the non-lookahead value would normally return results, will return their
results in the position where the LookAhead is included in the grammar.

If the processor argument to parse is true and callable, the processor object
will be called with ``(success, resultTrees, nextCharacter)`` on completion of
parsing. The processor can then take whatever processing steps desired, the
return value from calling the processor with the results is returned directly
to the caller of parse.

Byte Positions with Encoding Parameter
--------------------------------------

When using the ``encoding`` parameter to parse byte strings, all positions in
the result tree are *byte positions*, not character positions. This allows
direct slicing of the original byte string:

.. code-block:: python

    # Parse UTF-8 encoded bytes
    result = parser.parse(utf8_bytes, encoding='utf-8')
    success, trees, byte_position = result

    # Extract matched text using byte positions
    for (name, start, stop, children) in trees:
        matched_bytes = utf8_bytes[start:stop]
        matched_text = matched_bytes.decode('utf-8')

This is important when working with multi-byte encodings like UTF-8, where a
single character may occupy multiple bytes.

DispatchProcessor
-----------------

SimpleParse provides a simple mechanism for processing result trees, a
recursive series of calls to attributes of a "Processor" object with functions
to automate the call-by-name dispatching. This processor implementation is
available for examination in the ``simpleparse.dispatchprocessor`` module.

Utility Functions
~~~~~~~~~~~~~~~~~

The main functions are:

.. code-block:: python

    def dispatch(source, tag, buffer):
        """Dispatch on source for tag with buffer

        Find the attribute or key "tag-object" (tag[0]) of source,
        then call it with (tag, buffer)
        """

    def dispatchList(source, taglist, buffer):
        """Dispatch on source for each tag in taglist with buffer"""

    def multiMap(taglist, source=None, buffer=None):
        """Convert a taglist to a mapping from tag-object:[list-of-tags]

        For instance, if you have items of 3 different types, in any order,
        you can retrieve them all sorted by type with multimap(childlist)
        then access them by tagobject key.

        If source and buffer are specified, call dispatch on all items.
        """

    def singleMap(taglist, source=None, buffer=None):
        """Convert a taglist to a mapping from tag-object:tag,
        overwriting early with late tags. If source and buffer
        are specified, call dispatch on all items."""

    def getString((tag, left, right, sublist), buffer):
        """Return the string value of the tag passed"""

    def lines(start=None, end=None, buffer=None):
        """Return number of lines in buffer[start:end]"""

The ``DispatchProcessor`` class provides a ``__call__`` implementation to
trigger dispatching for both "called as root processor" and "called to process
an individual result element" cases.

Creating a Processor
~~~~~~~~~~~~~~~~~~~~

You define a DispatchProcessor sub-class with methods named for each production
that will be processed by the processor:

.. code-block:: python

    from simpleparse.dispatchprocessor import DispatchProcessor, getString, dispatch

    class MyProcessorClass(DispatchProcessor):
        def production_name(self, tup, buffer):
            """Process the given production and its children"""
            tag, start, stop, subtags = tup
            # Process as needed...
            return getString(tup, buffer)

Within those production-handling methods, you can call the dispatch functions
to process the sub-tags of the current production (keep in mind that the
sub-tags "list" may be a None object). You can see examples of this processing
methodology in:

* ``simpleparse.simpleparsegrammar``
* ``simpleparse.common.iso_date``
* ``simpleparse.common.strings``

Default Processor
~~~~~~~~~~~~~~~~~

For real-world Parsers, where you normally use the same processing class for
all runs of the parser, you can define a default Processor class like so:

.. code-block:: python

    class MyParser(Parser):
        def buildProcessor(self):
            return MyProcessorClass()

If no processor is explicitly specified in the parse call, your
``MyProcessorClass`` instance will be used for processing the results.

.. _nonstandardresulttrees:

Non-standard Result Trees
-------------------------

SimpleParse includes features which expose certain of the mx.TextTools library's
features for producing non-standard result trees: AppendMatch, AppendToTagobj,
AppendTagobj, and CallTag. Although not generally recommended for use in
"normal" parsers, these features are useful for certain types of text
processing, and their exposure was requested. Each flag has a different effect
on the result tree, the particular effects are discussed below.

The exposure is through the Processor (or more precisely, a super-class of
Processor called "MethodSource") object. To specify the use of one of the flags,
you set an attribute in your MethodSource object (your Processor object) with
the name ``_m_productionname`` (for the "method" to use, which is either an
actual callable object for use with CallTag, or one of the other mx.TextTools
flag constants above). In the case of AppendTagobj, you will likely want to
specify a particular tagobj object to be appended, you do that by setting an
attribute named ``_o_productionname`` in your MethodSource.

.. note::

    You can use MethodSource as your direct ancestor if you want to define a
    non-standard result tree, but don't want to do any processing of the results
    (this is the reason for having separate classes). MethodSource does not
    define a ``__call__`` method.

CallTag
~~~~~~~

.. code-block:: python

    _m_productionname = callableObject

The given callable is called on a successful match with:

.. code-block:: python

    callableObject(taglist, text, left, right, subtags)

The ``text`` argument is the entire text buffer being parsed, the rest of the
values are what you're accustomed to seeing in result tuples.

**Notes:**

* Nothing is (necessarily) added to the results list when CallTag is specified!
  If you want something added, call ``taglist.append(item)``.
* Raising an error in the CallTag method will halt parsing.
* The callableObject is accessed from the MethodSource object using standard
  getattr, so if you are using a function, it will need to define a "self"
  parameter for the first position.

AppendToTagobj
~~~~~~~~~~~~~~

.. code-block:: python

    _m_productionname = AppendToTagobj
    _o_productionname = objectWithAppendMethod

On a successful match, the system will call
``_o_productionname.append((None, l, r, subtags))`` method. For some processing
tasks, it's conceivable you might want to use this method to pull out all
instances of a production from a larger (already-written) grammar where going
through the whole results tree to find the deeply nested productions is
considered too involved.

**Notes:**

* Nothing is added to the results list when AppendToTagobj is specified!
* Raising an error in the AppendToTagobj method will halt parsing.

AppendMatch
~~~~~~~~~~~

.. code-block:: python

    _m_productionname = AppendMatch

On a successful match, the system will append the matched text to the result
tree, rather than a tuple of results. In situations where you just want to
extract the text, this can be useful. The downside is that your results tree
has a non-standard format that you need to explicitly watch out for while
processing the results.

AppendTagobj
~~~~~~~~~~~~

.. code-block:: python

    _m_productionname = AppendTagobj
    _o_productionname = any_object  # optional, defaults to production name string

On a successful match, the system will append the tagobject to the result tree,
rather than a tuple of results. In situations where you just want notification
that the production has matched (and it doesn't matter what it matched), this
can be useful. The downside, again, is that your results tree has a non-standard
format that you need to explicitly watch out for while processing the results.
