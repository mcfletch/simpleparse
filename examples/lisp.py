"""Basic LISP parser

We use library items, so we get " strings, float, int, and hex
atoms, as well as lists.  Note: Lisp doesn't seem to
use "," for seperating atoms in lists?  I don't really
remember it well enough to recall, but seems to match the
samples I see.

Note: Original grammar was from a sample in the YAPPS
documentation.  Though it's kinda hard to recognise here.
"""

from __future__ import print_function

definition = r"""
### A simple LISP parser

<ts>        := [ \t\n\r]*
<nameChar>  := [-+*/!@%^&=.a-zA-Z0-9_]
quote       := "'"
name        := nameChar+
>atom<      := quote / string_double_quote / list / number_expr / name

# numbers are regular number values followed
# by something that is _not_ a nameCharacter
number_expr := number, ?-(nameChar)
list        := "(", seq?, ")"!
>seq<       := ts, atom, (ts,atom)*, ts
"""
from simpleparse.parser import Parser
from simpleparse.common import strings, numbers
from simpleparse.dispatchprocessor import *

parser = Parser( definition, 'atom' )

if __name__ == "__main__":
    from simpleparse.stt.TextTools import print_tags

    shouldParse = [
        "(+ 2 3)",
        "(- 2 3)",
        "(* 2 3)",
        "(quote (2 3 4))",
        "(23s (2.4s 3s 45.3))",
        "(() () (2 3 4))",
        "()",
        '''("thisand that" ())''',
        '"this"',
        '''('"this")''',
        '''("this\n\r" ' those (+ a b) (23s 0xa3 55.3) "s")''',
        r'''("this\n\r" ' those (+ a b) (23s 0xa3 55.3) "s")''',
        r'''("this\n\r" ' those (+ a b) (23s 0xa3 55.3] "s")''',
        '''("this\n\r" ' those (+ a b) (23s 0xa3 55.3\n\n] "s")''',
        '''(with-pedantry :high It's "Scheme In One Defun".)''',

    ]
    import pprint
    for item in shouldParse:
        try:
            success, children, next = parser.parse( item )
            if not success:
                print('fail', item)
            else:
                print('success', item, next)
                pprint.pprint( children )
        except SyntaxError as err:
            print(err)
