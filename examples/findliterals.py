import os, string
from simpleparse.parser import Parser

declaration = r'''
myfile := (notliteral,literal)+, notliteral

# not-a-literal, not reported, repeating
<notliteral> := -literal*

literal             :=  ("'",(CHARNOSNGLQUOTE/ESCAPEDCHAR)*,"'")  /  ('"',(CHARNODBLQUOTE/ESCAPEDCHAR)*,'"')

CHARNOSNGLQUOTE     :=  -[\\']+
CHARNODBLQUOTE      :=  -[\\"]+
ESCAPEDCHAR         :=  '\\',( SPECIALESCAPEDCHAR / OCTALESCAPEDCHAR )
SPECIALESCAPEDCHAR  :=  [\\abfnrtv]
OCTALESCAPEDCHAR    :=  [0-7],[0-7]?,[0-7]?
'''
parser = Parser( declaration, "myfile" )

def bigtest( file, parser = parser  ):
	val = parser.parse( file)
	print 'parsed %s characters of %s characters' % (val[-1], len(file))
	return val

def test():
	bigtest( ''' "this" "that" "them" ''' )
	bigtest( ''' "this" 'that' "th'em" ''' )
	

usage =''' findliterals filename
Finds all single and double-quoted literals in a file and prints them to stdout.
Is not triple-quoted string aware.'''

if __name__ == '__main__':
	test()
	import sys
	if sys.argv[1:]:
		import time
		filename = sys.argv[1]
		file = open( filename ).read()
		t = time.time()
		val = bigtest( file )
		t = t-time.time()
		print '''Parsing Time:''', t
		for report, start, stop, children in val[1]:
			print string.split(file[ start: stop ], '\n')[0][:75]
	else:
		print usage
