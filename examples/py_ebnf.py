"""This module defines a parser for the EBNF format used to define Python's grammar

The grammar in the language reference (as seen in Python 2.2.1)
seems to be somewhat messed up.  I've tried to fix the glaring
errors (such as control codes included in the text version) and
have saved this in the examples directory.

This parser does parse the entire (fixed) grammar, with the
exception of the <XXX> style comments which were used in a few
places in the grammar to say "range of characters"

What this doesn't do, is try to _use_ the parsed grammar.  The
grammar is assuming a very different parser type than SimpleParse,
for instance, it assumes that alternation (|) will use longest-of
semantics, so that:

	int      := blah
	long_int := int, [lL]
	all_ints := int, long_int

Would match long_int (because it's longest), rather than int, which
is what the base SimpleParse FOGroup would do.  You could fairly
trivially make a processor similar to the simpleparsegrammar one
to make objectgenerator objects from the parsed format, but the
resulting parser just wouldn't work because of the differences in
parser capability.

Basically, we'll want to have a new back-end before continuing on
with this demo.

The grammar being parsed (and included) is part of Python, so
here's the copyright notice:

	Python is Copyright (c) 2001, 2002 Python Software Foundation.
	All Rights Reserved.

	Copyright (c) 2000 BeOpen.com.
	All Rights Reserved.

	Copyright (c) 1995-2001 Corporation for National Research Initiatives.
	All Rights Reserved.

	Copyright (c) 1991-1995 Stichting Mathematisch Centrum, Amsterdam.
	All Rights Reserved.
	
You should have a full copy of the Python license in your Python
distribution.
"""
declaration = r"""

declarationset      :=  declaration+
declaration         :=  ts, '\n'?, name ,ts,'::=',fo_group

>group<             :=  '(',fo_group, ')'
>fo_group_children<    := (seq_group/element_token)
fo_group            :=  ts,fo_group_children,
                          (ts, fo_indicator, ts,
                              fo_group_children
                          )*, ts

seq_group            :=  ts, element_token, (ts, element_token)+, ts

element_token       :=  (optional_element / base_element), repetition?

repetition          := ('*'/'+')
optional_element    := '[',fo_group,']'
>base_element<      := (range/string/group/name)
<fo_indicator>      :=  '|'

name                :=  [a-zA-Z_],[a-zA-Z0-9_]*
<ts>                :=  (
	('\n', ?-name) /
	[ \011]+ /
	comment
)*
comment             :=  '#',-'\n'+,'\n'

range               :=  string, ts, '...', ts, string

"""
from simpleparse.parser import Parser
from simpleparse.common import strings

parser = Parser( declaration )
if __name__ == "__main__":
	from simpleparse.stt.TextTools import print_tags
	grammar = open("""py_grammar.txt""").read()
	success, result, next = parser.parse( grammar, 'declarationset')
	print 'success', success, next
	print_tags( grammar, result )
