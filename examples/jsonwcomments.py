"""Simple example of a json parser with comment support"""
from __future__ import print_function

from simpleparse.common import numbers, strings, comments

declaration = r'''
object    := ts,'{',!,ts,(member,sep?)*,'}'

member    := string,ts,':',ts,value,ts
array     := ts,'[',!,ts,(value,sep?)*,']',ts

>value<     := js_string/object/array/true/false/null/float/int_w_exp/int

int_w_exp := int,[eE],int

>js_string< := string_single_quote/string_double_quote
true      := 'true'
false     := 'false'
null      := 'null'

<ts>      := ([ \t\n\r]+/comment)*
<comment> := slashslash_comment/c_nest_comment
<sep>     := (','?,ts)
'''
from simpleparse.parser import Parser
from simpleparse.dispatchprocessor import *
import pprint

class Processor( DispatchProcessor ):
    def object( self, info, buffer ):
        (tag,start,stop,children) = info
        obj = {}
        for key,value in dispatchList( self, children, buffer ):
            obj[key] = value
        return obj
    def member( self, info, buffer ):
        (tag,start,stop,children) = info
        return dispatchList( self, children, buffer )
    def array( self, info, buffer ):
        (tag,start,stop,children) = info
        return dispatchList( self, children, buffer )
    def int_w_exp( self, info, buffer ):
        (tag,start,stop,(a,b)) = info
        base = dispatch( self, a, buffer )
        exp = dispatch( self, b, buffer )
        return base ** exp
    int = numbers.IntInterpreter()
    float = numbers.FloatInterpreter()
    string = strings.StringInterpreter()
    string_double_quote = strings.StringInterpreter()
    string_single_quote = string_double_quote
    def true( self, tag, buffer ):
        return True
    def false( self, tag, buffer ):
        return False
    def null( self, tag, buffer ):
        return None


parser = Parser( declaration, "object" )
if __name__ =="__main__":
    import sys,json
    print(json.dumps(
        parser.parse( open(sys.argv[1]).read(), processor=Processor())
    ))
