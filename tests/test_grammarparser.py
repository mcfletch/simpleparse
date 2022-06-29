"""Tests that simpleparsegrammar does parse SimpleParse grammars
"""
import sys, unittest, pprint, re,functools as ft
from collections import deque
from simpleparse.simpleparsegrammar import SPGenerator, declaration, Parser as GrammarParser, SPGrammarProcessorBytes,SPGrammarProcessorStr
from simpleparse.parser import Parser
from simpleparse.stt.TextTools import TextTools
from .genericvalues import NullResult, AnyInt
from simpleparse.objectgenerator import Name

from simpleparse.stt.TextTools import print_tagtable
print_tagtable(
    SPGenerator.buildParser( 'range' )
)


exc_info = sys.exc_info
class SimpleParseGrammarTests(unittest.TestCase):
    """Test parsing of the the simpleparse grammar elements"""
    def doBasicTest(self, parserName, testValue, expected, ):
        parser = SPGenerator.buildParser( parserName )
        result = TextTools.tag( testValue, parser )
        assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( pprint.pformat(expected), pprint.pformat(result))
    def testChar1( self ):
        self.doBasicTest(
            "CHARNODBLQUOTE",
            b'test\\""',
            (1, [], 4),
        )
    def testChar2( self ):
        self.doBasicTest(
            "ESCAPEDCHAR",
            b'\\n"',
            (1, [('SPECIALESCAPEDCHAR', 1, 2, NullResult)], 2),
        )
    def testChar3( self ):
        self.doBasicTest(
            "ESCAPEDCHAR",
            b'\\007"',
            (1, [('OCTALESCAPEDCHAR', 1, 4, NullResult)], 4),
        )
    def testChar4( self ):
        testValue = b'\\""'
        self.doBasicTest(
            "CHARNODBLQUOTE",
            testValue,
            (0, [], AnyInt),
        )
    def testChar5( self ):
        self.doBasicTest(
            "CHARNODBLQUOTE",
            b'ehllo\\""',
            (1, [], 5),
        )
    def testChar6( self ):
        self.doBasicTest(
            "CHARNODBLQUOTE",
            b'007',
            (1, [], 3),
        )
    def testChar7( self ):
        self.doBasicTest(
            "ESCAPEDCHAR",
            b'\\"',
            (1, [('SPECIALESCAPEDCHAR', 1, 2, NullResult)], 2),
        )
    def testChar8( self ):
        self.doBasicTest(
            "ESCAPEDCHAR",
            b'\\"',
            (1, [('SPECIALESCAPEDCHAR', 1, 2, NullResult)], 2),
        )
    def testChar9( self ):
        self.doBasicTest(
            "ESCAPEDCHAR",
            b'\\x10',
            (1, [('HEXESCAPEDCHAR', 2, 4, NullResult)], 4),
        )
    def testChar85( self ):
        self.doBasicTest(
            "HEXESCAPEDCHAR",
            b'10',
            (1, [], 2),
        )
    def testCharNoBrace1( self ):
        self.doBasicTest(
            "CHARNOBRACE",
            b'a-z',
            (1, [('CHAR', 0, 1, NullResult)], 1),
        )
    def testCharRange1( self ):
        self.doBasicTest(
            "CHARRANGE",
            b'a-z',
            (1, [('CHARNOBRACE', 0, 1, [('CHAR', 0, 1, NullResult)]),('CHARNOBRACE', 2, 3, [('CHAR', 2, 3, NullResult)])], 3),
        )
    def testRange1( self ):
        self.doBasicTest(
            "range",
            b'[a-zA-Z]',
            (1, [
                ('CHARRANGE',1,4,[
                    ('CHARNOBRACE', 1, 2, [('CHAR', 1, 2, NullResult)]),
                    ('CHARNOBRACE', 3, 4, [('CHAR', 3, 4, NullResult)]),
                ]),
                ('CHARRANGE',4,7,[
                    ('CHARNOBRACE', 4, 5, [('CHAR', 4, 5, NullResult)]),
                    ('CHARNOBRACE', 6, 7, [('CHAR', 6, 7, NullResult)]),
                ]),
            ], 8)
        )
    def testRange2( self ):
        self.doBasicTest(
            "range",
            b'[-a-zA-Z]',
            (1, [
                ('CHARDASH', 1, 2, NullResult),
                ('CHARRANGE',2,5,[
                    ('CHARNOBRACE', 2, 3, [('CHAR', 2, 3, NullResult)]),
                    ('CHARNOBRACE', 4, 5, [('CHAR', 4, 5, NullResult)]),
                ]),
                ('CHARRANGE',5,8,[
                    ('CHARNOBRACE', 5, 6, [('CHAR', 5, 6, NullResult)]),
                    ('CHARNOBRACE', 7, 8, [('CHAR', 7, 8, NullResult)]),
                ]),
            ], 9),
        )
    def testRange3( self ):
        self.doBasicTest(
            "range",
            b'[]a-zA-Z]',
            (1, [
                ('CHARBRACE', 1, 2, NullResult),
                ('CHARRANGE',2,5,[
                    ('CHARNOBRACE', 2, 3, [('CHAR', 2, 3, NullResult)]),
                    ('CHARNOBRACE', 4, 5, [('CHAR', 4, 5, NullResult)]),
                ]),
                ('CHARRANGE',5,8,[
                    ('CHARNOBRACE', 5, 6, [('CHAR', 5, 6, NullResult)]),
                    ('CHARNOBRACE', 7, 8, [('CHAR', 7, 8, NullResult)]),
                ]),
            ], 9),
        )

    def testRange4( self ):
        """Test optional repeating children running into eof

        Original SimpleParse had a major failure here,
        system hung trying to parse the [] string.  Basically,
        there was no check for EOF during a repeating-item
        parse (save for literals and character sets), so you
        wound up with infinite loops.
        """
        self.doBasicTest(
            "range",
            b'[]',
            (0, [], AnyInt),
        )
    def testRange5( self ):
        """Test optional repeating children with no termination

        Original SimpleParse had a major failure here,
        system hung trying to parse the [] string.  Basically,
        there was no check for EOF during a repeating-item
        parse (save for literals and character sets), so you
        wound up with infinite loops.
        """
        self.doBasicTest(
            "range",
            b'[] ',
            (0, [], AnyInt),
        )
        
    def testLiteral1( self ):
        self.doBasicTest(
            "literal",
            b'"test"',
            (1, [('CHARNODBLQUOTE', 1, 5, NullResult)], 6),
        )
    def testLiteral2( self ):
        self.doBasicTest(
            "literal",
            b'"test\\""',
            (1, [
                ('CHARNODBLQUOTE', 1, 5, NullResult),
                ('ESCAPEDCHAR', 5, 7, [
                    ('SPECIALESCAPEDCHAR', 6, 7, NullResult)
                ])
            ], 8)
            
        )
    def testLiteral3( self ):
        self.doBasicTest(
            "literal",
            b'""',
            (1, [], 2),
        )
    def testLiteral4( self ):
        self.doBasicTest(
            "literal",
            b'"\'"',
            (1, [('CHARNODBLQUOTE', 1, 2, NullResult),], 3),
        )
    def testLiteral5( self ):
        self.doBasicTest(
            "literal",
            b'"\\"test"',
            (1, [
                ('ESCAPEDCHAR', 1, 3, [
                    ('SPECIALESCAPEDCHAR', 2, 3, NullResult)
                ]),
                ('CHARNODBLQUOTE', 3, 7, NullResult)
            ], 8)			
        )
    def testLiteral6( self ):
        self.doBasicTest(
            "literal",
            b'"test\\023""',
            (1, [
                ('CHARNODBLQUOTE', 1, 5, NullResult),
                ('ESCAPEDCHAR', 5, 9, [
                    ('OCTALESCAPEDCHAR', 6, 9, NullResult)
                ])
            ], 10)
            
        )
    def testLiteralDecorator( self ):
        self.doBasicTest(
            "literalDecorator",
            b'c',
            (1, [], 1),
        )
    def testLiteralDecorator2( self ):
        self.doBasicTest(
            "literal",
            b'c"this"',
            (1, [('literalDecorator',0,1,NullResult),('CHARNODBLQUOTE',2,6,NullResult)], 7),
        )
    def testLiteralDecorator3( self ):
        """Decorator must be right next to literal, no whitespace"""
        self.doBasicTest(
            "literal",
            b'c "this"',
            (0, [], AnyInt),
        )
        
    def testWhitespace1( self ):
        self.doBasicTest(
            "ts",
            b'  \t',
            (1, [], 3)
        )
    def testWhitespace2( self ):
        self.doBasicTest(
            "ts",
            b'  \t\n',
            (1, [], 4)
        )
    def testWhitespace3( self ):
        self.doBasicTest(
            "ts",
            b'  \t#testing\r\n',
            (1, [('comment', 3, 13, NullResult)], 13)
        )
    def testWhitespace4( self ):
        self.doBasicTest(
            "ts",
            b'nospace',
            (1, [], 0)
        )
    def testWhitespace5( self ):
        """Bug in 2.0.0 where Null comments such as:
        "#\n"

        didn't parse.
        """
        self.doBasicTest(
            "ts",
            b' #\n ',
            (1, [('comment',1,3,NullResult)], 4)
        )
        
    def testName1( self ):
        self.doBasicTest(
            "name",
            b'abcdefg',
            (1, [], 7)
        )
    def testName2( self ):
        self.doBasicTest(
            "name",
            b'2abcdefg',
            (0, [], AnyInt)
        )
    def testName3( self ):
        self.doBasicTest(
            "name",
            b'_abcdefg_-',
            (1, [], 9)
        )

    def testUnreportedName1( self ):
        self.doBasicTest(
            "unreportedname",
            b'<abcdefg>',
            (1, [('name',1,8,NullResult)], 9)
        )
    def testUnreportedName2( self ):
        self.doBasicTest(
            "unreportedname",
            b'<>',
            (0, [], AnyInt)
        )
    def testExpandedName1( self ):
        self.doBasicTest(
            "expandedname",
            b'>abcdefg<',
            (1, [('name',1,8,NullResult)], 9)
        )
    def testExpandedName2( self ):
        self.doBasicTest(
            "expandedname",
            b'><',
            (0, [], AnyInt)
        )
    def testComment1( self ):
        self.doBasicTest(
            "comment",
            b'>',
            (0, [], AnyInt)
        )
    def testComment2( self ):
        self.doBasicTest(
            "comment",
            b'#testing\n',
            (1, [], 9)
        )
    def testOccurenceIndicator1( self ):
        self.doBasicTest(
            "occurence_indicator",
            b'*',
            (1, [], 1)
        )
    def testOccurenceIndicator2( self ):
        self.doBasicTest(
            "occurence_indicator",
            b'+',
            (1, [], 1)
        )
    def testOccurenceIndicator3( self ):
        self.doBasicTest(
            "occurence_indicator",
            b'?',
            (1, [], 1)
        )
    def testOccurenceIndicator4( self ):
        self.doBasicTest(
            "occurence_indicator",
            b'hello',
            (0, [], AnyInt)
        )
    def testOccurenceIndicator5( self ):
        self.doBasicTest(
            "occurence_indicator",
            b'',
            (0, [], AnyInt)
        )

    def testLookAheadIndicator1( self ):
        self.doBasicTest(
            "lookahead_indicator",
            b'?',
            (1, [], 1)
        )
    def testLookAheadIndicator2( self ):
        self.doBasicTest(
            "lookahead_indicator",
            b'',
            (0, [], AnyInt)
        )

    def testNegposIndicator1( self ):
        self.doBasicTest(
            "negpos_indicator",
            b'-',
            (1, [], 1)
        )
    def testNegposIndicator2( self ):
        self.doBasicTest(
            "negpos_indicator",
            b'+',
            (1, [], 1)
        )
    def testNegposIndicator3( self ):
        self.doBasicTest(
            "negpos_indicator",
            b')',
            (0, [], AnyInt)
        )
    def testErrorOnFailFlag1( self ):
        self.doBasicTest(
            "error_on_fail",
            b'!',
            (1, [], 1)
        )
    def testFOGroup1( self ):
        self.doBasicTest(
            "fo_group",
            b'a/b',
            (1, [
                ('element_token', 0,1,[
                    ("name",0,1,NullResult),
                ]),
                ('element_token', 2,3,[
                    ("name",2,3,NullResult),
                ]),
            ], 3)
        )
    def testSEQToken1( self ):
        self.doBasicTest(
            "seq_group",
            b'a,b',
            (1, [
                ('element_token', 0,1,[
                    ("name",0,1,NullResult),
                ]),
                ('element_token', 2,3,[
                    ("name",2,3,NullResult),
                ]),
            ], 3)
        )
    def testSEQGroup1( self ):
        self.doBasicTest(
            "seq_group",
            b'a,#c\012b',
            (1, [
                ('element_token', 0,1,[
                    ("name",0,1,NullResult),
                ]),
                ('element_token', 5,6,[
                    ("name",5,6,NullResult),
                ]),
            ], 6)
        )
    def testSeqGroup2( self ):
        self.doBasicTest(
            "seq_group",
            b'ts, (unreportedname/expandedname/name)',
            (1, [
                ('element_token', 0,2,[
                    ("name",0,2,NullResult),
                ]),
                ('element_token', 4,38,[
                    ('seq_group',5,37,[
                        ('fo_group',5,37,[
                            ('element_token', 5,19,[
                                ("name",5,19,NullResult),
                            ]),
                            ('element_token', 20,32,[
                                ("name",20,32,NullResult),
                            ]),
                            ('element_token', 33,37,[
                                ("name",33,37,NullResult),
                            ]),
                        ]),
                    ]),
                ]),
            ], 38)
        )
    def testSeqGroup3( self ):
        self.doBasicTest(
            "seq_group",
            b'(a/b/c)',
            (1, [
                ('element_token',0,7,[
                    ('seq_group',1,6,[
                        ('fo_group',1,6,[
                            ('element_token', 1,2,[
                                ("name",1,2,NullResult),
                            ]),
                            ('element_token', 3,4,[
                                ("name",3,4,NullResult),
                            ]),
                            ('element_token', 5,6,[
                                ("name",5,6,NullResult),
                            ]),
                        ]),
                    ]),
                ]),
            ], 7)
        )
    def testGroup1( self ):
        self.doBasicTest(
            "group",
            b'()',
            (0, [], AnyInt)
        )
    def testGroup2( self ):
        self.doBasicTest(
            "group",
            b'(hello)',
            (1, [
                ('seq_group',1,6,[
                    ('element_token', 1,6,[
                        ("name",1,6,NullResult),
                    ]),
                ]),
            ], 7)
        )
    def testGroup3( self ):
        '''Test group with sequential added group
        Note that this test also serves to test
        the function of non-reporting names'''
        self.doBasicTest(
            "group",
            b'(hello, there)',
            (1, [
                ('seq_group', 1,13,[
                    ('element_token', 1,6,[
                        ("name",1,6,NullResult),
                    ]),
                    ('element_token', 8,13,[
                        ("name",8,13,NullResult),
                    ]),
                ]),
            ], 14)
        )
    def testGroup4( self ):
        '''Test group with sequential added group
        Note that this test also serves to test
        the function of non-reporting names'''
        self.doBasicTest(
            "group",
            b'(hello/there)',
            (1, [
                ('seq_group',1,12,[
                    ('fo_group',1,12,[
                        ('element_token', 1,6,[
                            ("name",1,6,NullResult),
                        ]),
                        ('element_token', 7,12,[
                            ("name",7,12,NullResult),
                        ]),
                    ]),
                ]),
            ], 13)
        )
    def testGroup5( self ):
        '''Test group with sequential added group
        Note that this test also serves to test
        the function of non-reporting names'''
        self.doBasicTest(
            "group",
            b'([the]/"and")',
            (1, [
                ('seq_group',1,12,[
                    ('fo_group',1,12,[
                        ('element_token', 1,6,[
                            ("range",1,6,[
                                ('CHARNOBRACE', 2,3,[ # this should really be a collapsed level
                                    ('CHAR', 2,3,NullResult),
                                ]),
                                ('CHARNOBRACE', 3,4,[ # this should really be a collapsed level
                                    ('CHAR', 3,4,NullResult),
                                ]),
                                ('CHARNOBRACE', 4,5,[ # this should really be a collapsed level
                                    ('CHAR', 4,5,NullResult),
                                ]),
                            ]),
                        ]),
                        ('element_token', 7,12,[
                            ("literal",7,12,[
                                ('CHARNODBLQUOTE', 8,11,NullResult),
                            ]),
                        ]),
                    ]),
                ]),
            ], 13)
        )
    def testGroup6( self ):
        '''Test group with multiple / 'd values'''
        self.doBasicTest(
            "group",
            b'(hello/there/a)',
            (1, [
                ('seq_group',1,14,[
                    ('fo_group',1,14,[
                        ('element_token', 1,6,[
                            ("name",1,6,NullResult),
                        ]),
                        ('element_token', 7,12,[
                            ("name",7,12,NullResult),
                        ]),
                        ('element_token', 13,14,[
                            ("name",13,14,NullResult),
                        ]),
                    ]),
                ]),
            ], 15)
        )
    def testElementToken1( self ):
        self.doBasicTest(
            "element_token",
            b'hello',
            (1, [
                ("name",0,5,NullResult),
            ], 5)
        )
    def testElementToken2( self ):
        self.doBasicTest(
            "element_token",
            b'-hello',
            (1, [
                ("negpos_indicator",0,1,NullResult),
                ("name",1,6,NullResult),
            ], 6)
        )
    def testElementToken3( self ):
        self.doBasicTest(
            "element_token",
            b'-hello?',
            (1, [
                ("negpos_indicator",0,1,NullResult),
                ("name",1,6,NullResult),
                ("occurence_indicator",6,7,NullResult),
            ], 7)
        )
    def testElementToken4( self ):
        self.doBasicTest(
            "element_token",
            b'- hello ?',
            (1, [
                ("negpos_indicator",0,1,NullResult),
                ("name",2,7,NullResult),
                ("occurence_indicator",8,9,NullResult),
            ], 9)
        )
    def testElementToken5( self ):
        self.doBasicTest(
            "element_token",
            b'+ hello ?',
            (1, [
                ("negpos_indicator",0,1,NullResult),
                ("name",2,7,NullResult),
                ("occurence_indicator",8,9,NullResult),
            ], 9)
        )
    def testElementToken6( self ):
        """Lookahead indicator with positive"""
        self.doBasicTest(
            "element_token",
            b'? + hello ?',
            (1, [
                ("lookahead_indicator",0,1,NullResult),
                ("negpos_indicator",2,3,NullResult),
                ("name",4,9,NullResult),
                ("occurence_indicator",10,11,NullResult),
            ], 11)
        )
    def testElementToken7( self ):
        """Lookahead indicator with negative"""
        self.doBasicTest(
            "element_token",
            b'? - hello ?',
            (1, [
                ("lookahead_indicator",0,1,NullResult),
                ("negpos_indicator",2,3,NullResult),
                ("name",4,9,NullResult),
                ("occurence_indicator",10,11,NullResult),
            ], 11)
        )
    def testElementToken8( self ):
        """Lookahead indicator with no neg or pos"""
        self.doBasicTest(
            "element_token",
            b'?hello?',
            (1, [
                ("lookahead_indicator",0,1,NullResult),
                ("name",1,6,NullResult),
                ("occurence_indicator",6,7,NullResult),
            ], 7)
        )
    def testElementToken9( self ):
        """Error on fail indicator"""
        self.doBasicTest(
            "element_token",
            b'hello+!',
            (1, [
                ("name",0,5,NullResult),
                ("occurence_indicator",5,6,NullResult),
                ("error_on_fail",6,7,NullResult),
            ], 7)
        )
    def testElementToken10( self ):
        """Error on fail indicator with message"""
        self.doBasicTest(
            "element_token",
            b'hello+! "Unable to complete parse, yikes!"',
            (1, [
                ("name",0,5,NullResult),
                ("occurence_indicator",5,6,NullResult),
                ("error_on_fail",6,42,[
                    ("literal",8,42,[
                        ("CHARNODBLQUOTE",9,41,NullResult),
                    ]),
                ]),
            ], 42)
        )
    def testCutToken2( self ):
        self.doBasicTest(
            "element_token",
            b'(!,a)',
            (1, [
                ('seq_group', 1,4, [
                    ("error_on_fail",1,2,NullResult),
                    ('element_token',3,4,[
                        ("name",3,4,NullResult),
                    ]),
                ]),
            ], 5)
        )
    def testCutToken3( self ):
        self.doBasicTest(
            "element_token",
            b'(a,!"this")',
            (1, [
                ('seq_group', 1,10, [
                    ('element_token',1,2,[
                        ("name",1,2,NullResult),
                    ]),
                    ("error_on_fail",3,10,[
                        ("literal",4,10,[
                            ("CHARNODBLQUOTE",5,9,NullResult),
                        ]),
                    ]),
                ]),
            ], 11)
        )
    def testCutToken4( self ):
        self.doBasicTest(
            "element_token",
            b'(a,!"this",b)',
            (1, [
                ('seq_group', 1,12, [
                    ('element_token',1,2,[
                        ("name",1,2,NullResult),
                    ]),
                    ("error_on_fail",3,10,[
                        ("literal",4,10,[
                            ("CHARNODBLQUOTE",5,9,NullResult),
                        ]),
                    ]),
                    ('element_token',11,12,[
                        ("name",11,12,NullResult),
                    ]),
                ]),
            ], 13)
        )
    def testDeclaration( self ):
        self.doBasicTest(
            "declaration",
            b'a := "a"',
            (1, [
                ("name",0,1,NullResult),
                ('seq_group',4,8,[
                    ('element_token', 5,8,[
                        ("literal",5,8,[
                            ('CHARNODBLQUOTE', 6,7,NullResult),
                        ]),
                    ]),
                ]),
            ], 8)
        )
    def testDeclaration2( self ):
        self.doBasicTest(
            "declaration",
            b'a := b',
            (1, [
                ("name",0,1,NullResult),
                ('seq_group',4,6,[
                    ('element_token', 5,6,[
                        ("name",5,6,NullResult),
                    ])
                ]),
            ], 6)
        )
    def testDeclaration3( self ):
        self.doBasicTest(
            "declaration",
            b'a := ',
            (0,[],AnyInt)
        )
    def testDeclaration4( self ):
        self.doBasicTest(
            "declaration",
            b'<a> := b',
            (1, [
                ("unreportedname",0,3,[
                    ("name",1,2,NullResult),
                ]),
                ('seq_group',6,8,[
                    ('element_token', 7,8,[
                        ("name",7,8,NullResult),
                    ]),
                ])
            ], 8)
        )
    def testDeclaration5( self ):
        self.doBasicTest(
            "declaration",
            b'>a< := b',
            (1, [
                ("expandedname",0,3,[
                    ("name",1,2,NullResult),
                ]),
                ('seq_group',6,8,[
                    ('element_token', 7,8,[
                        ("name",7,8,NullResult),
                    ])
                ]),
            ], 8)
        )
    def testDeclarationSet1( self ):
        self.doBasicTest(
            "declarationset",
            b'a := b  #hello\012b:="c"',
            (1, [
                ('declaration', 0,15,[
                    ("name",0,1,NullResult),
                    ('seq_group',4,15,[
                        ('element_token', 5,15,[
                            ("name",5,6,NullResult),
                        ])
                    ])
                ]),
                ('declaration', 15,21,[
                    ("name",15,16,NullResult),
                    ('seq_group',18,21,[
                        ('element_token', 18,21,[
                            ("literal",18,21,[
                                ('CHARNODBLQUOTE', 19,20,NullResult),
                            ]),
                        ]),
                    ]),
                ]),
            ], 21)
        )
    def testDeclarationSet2( self ):
        '''Just tries to parse and sees that everything was parsed, doesn't predict the result'''
        parser = SPGenerator.buildParser( "declarationset" )
        result = TextTools.tag( declaration, parser )
        assert result[-1] == len(declaration), '''Didn't complete parse of the simpleparse declaration, only got %s chars, should have %s'''%(result[-1], len(declaration))


grammar_cutter = re.compile(rb"(?:^|[\n\r\f])(?P<cut>[<>]?(?P<name>\w+)[<>]?)\s*:?:=")

class SimpleParseGrammarProcessorBytesTests(SimpleParseGrammarTests):
    #@classmethod
    #def setUpClass(cls):
    #    cls.declarations = {
    #        match.group('name').decode('ascii'):declaration[match.start('cut'):]
    #        for match in grammar_cutter.finditer(declaration)
    #    }

    encode = bytes
    processor_cls = SPGrammarProcessorBytes
   
    def doBasicTest(self, parserName, testValue, expected, ):
        expected_product = SPGenerator.getRootObject(parserName)
        processor = self.processor_cls()
        parser = SPGenerator.buildParser('declarationset')
        taglist = TextTools.tag( declaration, parser,0,len(declaration) )
        got_tags = processor(taglist,declaration)
        result = processor.generator.getRootObject(parserName)
        compare = deque()
        compare.append((expected_product,result))
        decode = self.encode
        attrnames = ( 'negative','optional','repeating','lookahead','ErrorOnFail','report','expanded','requiresExpandedSet')
        # 'ts' in baseparser has also report on name reference set to 0 why ever so handle special
        name_ts_attrs = ( 'negative','optional','repeating','lookahead','ErrorOnFail','expanded','requiresExpandedSet')
        while compare:
            expect,got = compare.popleft()
            assert type(expect) == type(got)
            for attribute in (name_ts_attrs if ( isinstance(expect,Name) and expect.value == 'ts') or parserName == 'ts' else attrnames):
                assert getattr(expect,attribute,None) == getattr(got,attribute,None),'''\nexpected (flags missmatch):%s\n     got:%s\n'''%( expected_product, result )
            assert expect.terminal(SPGenerator) == got.terminal(processor.generator), '''\nexpected (terminal missmatch):%s\n     got:%s\n'''%( expected_product, result )
            expect_children = getattr(expect,'children',None)
            if expect_children is None:
                assert getattr(got,'children',None) is None , '''\nexpected (no children):%s\n     got:%s\n'''%( expected_product, result )
            else:
                assert getattr(got,'children',None) is not None, '''\nexpected (have children):%s\n     got:%s\n'''%( expected_product, result )
                assert len(expect.children) == len(got.children), '''\nexpected (%i children):%s\n     got (%i children):%s\n'''%( len(expect.children),expected_product,len(got.children),result )
                compare.extend(zip(expect.children,got.children))
            expect_value = getattr(expect,'value',None)
            if expect_value is None:
                assert getattr(got,'value',None) is None
            elif isinstance(expect,Name):
                assert getattr(got,'value') == expect_value,'''\nexpected (name string):%s\n     got:%s\n'''%( expected_product, result )
            else:
                assert getattr(got,'value',None) == decode(expect_value), '''\nexpected (%s value):%s\n     got:%s\n'''%(type(decode), expected_product, result )

class SimpleParseGrammarProcessorStrTests(SimpleParseGrammarProcessorBytesTests):
    encode = ft.partial(str,encoding='utf8')
    processor_cls = SPGrammarProcessorStr

class SimpleParseGrammarParserBytesTests(SimpleParseGrammarProcessorBytesTests):
                
    def doBasicTest(self, parserName, testValue, expected, ):
        generator = GrammarParser(declaration).generator
        parser = generator.buildParser(parserName)
        result = TextTools.tag( testValue, parser )
        assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )


utf8_declaration = declaration.decode('utf8')
class SimpleParseGrammarParserStrTests(SimpleParseGrammarTests):
    def doBasicTest(self, parserName, testValue, expected, ):
        generator = GrammarParser(utf8_declaration).generator
        parser = generator.buildParser(parserName)
        result = TextTools.tag( testValue.decode('utf8'), parser )
        assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )
    

class SimpleParseRecursiveTests(SimpleParseGrammarTests):
    """Test parsing of grammar elements with generated version of simpleparse grammar"""
    @classmethod
    def setUpClass(cls):
        cls.recursiveParser = Parser(declaration)

    def doBasicTest(self, parserName, testValue, expected, ):
        result = self.recursiveParser.parse( testValue, production=parserName )
        assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )

def getSuite():
    return unittest.TestSuite((
        unittest.makeSuite(SimpleParseGrammarTests,'test'),
        unittest.makeSuite(SimpleParseGrammarProcessorBytesTests,'test'),
        unittest.makeSuite(SimpleParseGrammarProcessorStrTests,'test'),
        unittest.makeSuite(SimpleParseGrammarParserBytesTests,'test'),
        unittest.makeSuite(SimpleParseGrammarParserStrTests,'test'),
        unittest.makeSuite(SimpleParseRecursiveTests,'test'),
    ))

if __name__ == "__main__":
    unittest.main(defaultTest="getSuite")
