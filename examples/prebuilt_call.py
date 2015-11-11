"""Example using pre-built "re" parsing object

The Pre-built Element Token lets you include elements
which cannot be readily defined in the SimpleParse EBNF
including items defined by a callout to a Python
function.  This example demonstrates the technique.

The example also (obviously) demonstrates the use of an
re object during the parsing process.
"""
from __future__ import print_function

import re
from simpleparse.stt.TextTools.TextTools import *
from simpleparse.parser import Parser
from simpleparse import dispatchprocessor
try:
    raw_input
except NameError:
    raw_input = input

class REMatch:
    """An object wrapping a regular expression with __call__ (and Call) semantics"""
    def __init__( self, expression, flags=0 ):
        self.matcher = re.compile( expression, flags )
    def __call__( self, text, position, endPosition ):
        """Return new text position, if > position, then matched, otherwise fails"""
        result = self.matcher.match( text, position, endPosition)
        if result:
            return result.end()
        else:
            # doesn't necessarily mean it went forward, merely
            # that it was satisfied, which means that an optional
            # satisfied but un-matched re will just get treated
            # like an error :(
            return position
    def table( self ):
        """Build the TextTools table for the object"""
        return ( (None, Call, self ), )

declaration = r"""
v :=  white?,(word,white?)+
"""

class WordProcessor( dispatchprocessor.DispatchProcessor ):
    """Processor sub-class defining processing functions for the productions"""
    # you'd likely provide a "resetBeforeParse" method
    # in a real-world application, but we don't store anything
    # in our parser.
    def word( self, tup, buffer ):
        """Deal with a "word" production by printing out value"""
        print("word: ", repr(dispatchprocessor.getString(tup, buffer)))
    def white( self, tup, buffer ):
        """Deal with a "white" production by printing out value"""
        print("white:", repr(dispatchprocessor.getString(tup, buffer)))


parser = Parser( declaration, "v", prebuilts = [
    ("word", REMatch( "\w+").table()),
    ("white", REMatch( "\W+").table()),
])

if __name__ == "__main__":
    print("""Please enter some number of words seperated by whitespace.
We will attempt to parse them and return the parse results""")
    data = raw_input( ">>> " )
    parser.parse( data , processor = WordProcessor())
