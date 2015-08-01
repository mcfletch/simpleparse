"""Demonstrates what happens when your declaration is syntactically incorrect

When run as a script, will generate a traceback
telling you that the grammar defined here is
incorrectly formatted.
"""
from simpleparse.common import numbers, strings, comments

declaration = r'''# note use of raw string when embedding in python code...
file           :=  [ \t\n]*, section+
section        :=  '[',identifier,']' ts,'\n', body
body           :=  statement*
statement      :=  (ts,semicolon_comment)/equality/nullline
nullline       :=  ts,'\n'
comment        :=  -'\n'*
equality       :=  ts, identifier,ts,'=',ts,identified,ts,'\n'
identifier     :=  [a-zA-Z], [a-zA-Z0-9_]*
identified     :=  string/number/identifier
ts             :=  [ \t]*
'''

testdata = '''[test1]
    val=23
'''
if __name__ == "__main__":
    from simpleparse.parser import Parser
    parser = Parser( declaration, "file" ) # will raise ValueError
