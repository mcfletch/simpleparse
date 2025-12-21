Parser Class
============

The main Parser class for SimpleParse.

.. automodule:: simpleparse.parser
   :members:
   :undoc-members:
   :show-inheritance:

Usage Examples
--------------

Basic Parsing
~~~~~~~~~~~~~

.. code-block:: python

    from simpleparse.parser import Parser

    grammar = '''
    root := (word / space)+
    word := [a-zA-Z]+
    space := [ ]+
    '''

    parser = Parser(grammar, 'root')
    success, results, position = parser.parse("hello world")

Parsing with Encoding
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

    # Parse UTF-8 bytes
    text = 'hello 世界'.encode('utf-8')
    success, results, byte_pos = parser.parse(text, encoding='utf-8')

    # byte_pos is byte position (12), not character position (8)

Using Pre-built Parsers
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

    from simpleparse.common import numbers, strings

    grammar = '''
    assignment := identifier, '=', value
    identifier := [a-zA-Z_]+
    value := number / string
    '''

    parser = Parser(grammar, 'assignment')

Custom Processor
~~~~~~~~~~~~~~~~

.. code-block:: python

    from simpleparse.dispatchprocessor import DispatchProcessor, getString

    class MyProcessor(DispatchProcessor):
        def word(self, tup, buffer):
            return getString(tup, buffer).upper()

    parser = Parser(grammar, 'root')
    result = parser.parse("hello world", processor=MyProcessor())

Result Format
-------------

Parse results are tuples of the form::

    (production_name, start, stop, children)

* **production_name** (str): Name of the matched production
* **start** (int): Start position in the input
* **stop** (int): End position in the input
* **children** (list or None): Child result tuples
