Common Patterns
===============

Common parsing patterns and utilities.

This package contains pre-built parsers for commonly needed patterns.

Usage
-----

Import the modules you need before defining your grammar:

.. code-block:: python

    from simpleparse.common import numbers, strings, comments

This makes the common productions available to all subsequent Parser instances.

Available Modules
-----------------

Numbers Module
~~~~~~~~~~~~~~

The ``simpleparse.common.numbers`` module provides:

* ``int`` / ``integer`` - Integer numbers
* ``float`` / ``floatnumber`` - Floating point numbers
* ``number`` - Either int or float
* ``hex`` - Hexadecimal numbers
* ``binary_number`` - Binary numbers

.. code-block:: python

    from simpleparse.common import numbers

    grammar = '''
    value := number
    '''

Strings Module
~~~~~~~~~~~~~~

The ``simpleparse.common.strings`` module provides:

* ``string`` - Python-style strings with escapes
* ``simpleString`` - Simple quoted strings

.. code-block:: python

    from simpleparse.common import strings

    grammar = '''
    value := string
    '''

Comments Module
~~~~~~~~~~~~~~~

The ``simpleparse.common.comments`` module provides:

* ``c_comment`` - C-style ``/* ... */`` comments
* ``hash_comment`` - Python-style ``# ...`` comments
* ``semicolon_comment`` - INI-style ``; ...`` comments

.. code-block:: python

    from simpleparse.common import comments

    grammar = '''
    line := statement, semicolon_comment?
    '''

ISO Date Module
~~~~~~~~~~~~~~~

The ``simpleparse.common.iso_date`` module provides ISO 8601 date parsing.

Character Types Module
~~~~~~~~~~~~~~~~~~~~~~

The ``simpleparse.common.chartypes`` module provides character classification
patterns like ``uppercase``, ``lowercase``, ``digits``, etc.
