"""Re-written version of simpleexample for 2.0"""
from simpleparse.common import numbers, strings, comments

declaration = r'''# note use of raw string when embedding in python code...
file           :=  [ \t\n]*, section+
section        :=  '[',identifier,']', ts,'\n', body
body           :=  statement*
statement      :=  (ts,semicolon_comment)/equality/nullline
nullline       :=  ts,'\n'
equality       :=  ts, identifier,ts,'=',ts,identified,ts,'\n'
identifier     :=  [a-zA-Z], [a-zA-Z0-9_]*
identified     :=  string/number/identifier
ts             :=  [ \t]*
'''
testData = """[test1]
    val=23
    val2="23"
    val3 = "23\t\nskidoo\xee"
    wherefore="art thou"
    ; why not
    log = heavy_wood

[test2]
loose=lips
"""
from simpleparse.parser import Parser
import pprint

parser = Parser( declaration, "file" )
if __name__ =="__main__":
    pprint.pprint( parser.parse( testData))