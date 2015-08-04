"""A simple example of parsing

I have no idea for whom I originally created this code,
(which was originally written for SimpleParse 1.0) nor
why they wanted it.  Oh well, such is life.

Running as a script will do some timing tests, but the
tests are rather... simplistic.

The grammar is slow parsing around 5-10% of the speed I
normally expect from SimpleParse/mxTextTools parsers.
I'm guessing it gets into lots and lots of partial parses
of the "interesting" production, and that the huge number
of reported productions slows it down.  For example,
making atom non-reporting gives a 15% speedup on my
machine.
"""

from __future__ import print_function

declaration = r'''
set       := (interesting/multset/plusset)+
multset   := '*',(set/atom), (set/atom)
plusset   := '+',(set/atom), (set/atom)
atom      := -[+*]

>interesting< := (example8/example7/example6/example5/example4/example3/example2/example1)
example1     := '*+',(set/atom),(set/atom),'+',(set/atom),(set/atom)
example2     := '**',(set/atom),(set/atom),'++',(set/atom),(set/atom),(set/atom)
example3     := 'fsd*',(set/atom),(set/atom),'++',(set/atom),(set/atom),(set/atom)
example4     := 'm*',(set/atom),(set/atom),'++',(set/atom),(set/atom),(set/atom)
example5     := 'a*',(set/atom),(set/atom),'++',(set/atom),(set/atom),(set/atom)
example6     := 's*',(set/atom),(set/atom),'++',(set/atom),(set/atom),(set/atom)
example7     := 'bdf*',(set/atom),(set/atom),'++',(set/atom),(set/atom),(set/atom)
example8     := 'sd*',(set/atom),(set/atom),'++',(set/atom),(set/atom),(set/atom)
'''
import sys
from simpleparse.parser import Parser
parser = Parser(declaration,'set')


class Emitter:
    def process( self, data ):
        #import pprint
        tree = self.parse( data )
        #pprint.pprint( tree )
        # wrap up the tuple 'cause TextTools uses a different format for the top-level :(
        tree = ('set',0, tree[-1], tree[1] )
        return self.emit( tree )
    def parse( self, data ):
        self.data = data
        return parser.parse( data)
    def write( self, data ):
        sys.stdout.write( data )
    def emit( self, tree ):
        '''
        return transformation for a single tuple...
        '''
        if hasattr( self, 'emit' + tree[0] ): # have explicitprocessing function
            func = getattr( self, 'emit'+tree[0] )
            return func( tree )
        else:
            if tree[3]: # children to process, things to do :)
                result = []
                ### write out pre-elements
                endpos = tree[3][0][1] # start of first child
                result.append( self.data[ tree[1]:endpos] )
                ### write children
                for child in tree[3]:
                    result.append( self.emit( child ) )
                ### write out post elements
                startpos = tree[3][-1][2] # end of last child
                result.append( self.data[ startpos: tree[2]] )
                return ''.join(result)
            else:
                # we're just re-emitting same text...
                return self.data[ tree[1]:tree[2]]
    def emitexample1( self, tuple ):
        '''*+AB+CD -> ++*AC*AD+*BC*BD'''
        #print 'interesting'
        #import pdb
        #pdb.set_trace()
        a,b,c,d = list(map( self.emit, tuple[3] ))
        #print `(a,b,c,d)`,
        return '++*%s%s*%s%s+*%s%s*%s%s'%( a,c,a,d,b,c,b,d)

if __name__ == "__main__":

    testdata = [
    '''++m*++mkp+f*nkf''',
    '''*+ab+cd''',
    '''+ab+bc+de''',
    '''*ab*bc*de''',
    '''++m*++mkp+f*nkf'''*10000,
    ]

    a = Emitter()
    import time, profile
    for test in testdata:
        t = time.time()
        a.parse( test )
        t = time.time()-t
        print('total time', t, 'length', len(test))
        if t:
            print('  %s cps' % (len(test)/t))
