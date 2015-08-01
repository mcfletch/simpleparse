declaration = r'''# note use of raw string when embedding in python code...
file           :=  [ \t\n]*, section+
section        :=  '[',identifier,']', ts,'\n', body
body           :=  statement*
statement      :=  (ts,';',comment,'\n')/equality/nullline
nullline       :=  ts,'\n'
comment        :=  -'\n'*
equality       :=  ts, identifier,ts,'=',ts,identified,ts,'\n'
identifier     :=  [a-zA-Z], [a-zA-Z0-9_]*
identified     :=  ('"',string,'"')/number/identifier
ts             :=  [ \t]*
char           :=  -[\134"]+
number         :=  [0-9eE+.-]+
string         :=  (char/escapedchar)*
escapedchar    :=  '\134"' / '\134\134'
'''
testdata = '''[test1]
    val=23
    val2="23"
    wherefore="art thou"
    ; why not
    log = heavy_wood

[test2]
loose=lips

'''
from simpleparse.parser import Parser
import pprint

parser = Parser( declaration, "file" )
if __name__ =="__main__":
    pprint.pprint( parser.parse( testdata))