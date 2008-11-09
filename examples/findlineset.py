declaration = r"""

firstLine := "This is first line"
secondLine := "This is second line"
fifthLine := "This is fifth line"

<ts>  := [ \t]*

# the actual text strings are included directly
# for the negative versions which is basically to
# avoid the overhead of the name-ref indirection
# (which should be optimised away automatically, but isn't)
set := -firstLine*, firstLine, -secondLine*, secondLine, -fifthLine*, fifthLine
sets := set*
"""

from simpleparse.parser import Parser

p = Parser( declaration, 'set' )

file1 = """This is first line
This is second line
This is NOT first line
This is NOT second line
This is fifth line
This is NOT fifth line
"""
file2 = """This is first line
This is fifth line
This is second line
This is NOT first line
This is NOT second line
This is NOT fifth line
"""
if __name__ == "__main__":
    import pprint
    import time
    pprint.pprint(
        p.parse( file1)
    )
    pprint.pprint(
        p.parse( file2)
    )
    testData = "\n"*30000000 + file1
    print 'starting parse of file 1 with 1 match at end'
    t = time.clock()
    success, results, next = p.parse( testData, "sets")
    print 'finished parse', time.clock()-t
    print 'number of results', len(results)
    pprint.pprint(
        results
    )
    print
    testData = file1 * (30000000/len(file1))
    print 'starting parse of file 1 with ~230,000 matches (slow)'
    t = time.clock()
    success, results, next = p.parse( testData, "sets")
    print 'finished parse', time.clock()-t
    print 'number of results', len(results)

