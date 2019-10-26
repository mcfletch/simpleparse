"""Test the print-to-python-file module

This just uses the simpleparsegrammar declaration, which is
parsed, then linearised, then loaded as a Python module.
"""
import os, unittest, shutil
from . import test_grammarparser
try:
    reload
except NameError:
    from importlib import reload
HERE = os.path.dirname(__file__)

class PrintersTests(test_grammarparser.SimpleParseGrammarTests):
    def setUp( self ):
        from simpleparse import simpleparsegrammar, parser, printers, baseparser
        p = parser.Parser( simpleparsegrammar.declaration, 'declarationset')
        ns = {}
        exec(printers.asGenerator( p._generator ), ns, ns)
        class RParser( ns['Parser'], baseparser.BaseParser ):
            pass
        self.recursiveParser = RParser()
    def doBasicTest(self, parserName, testValue, expected, ):
        result = self.recursiveParser.parse( testValue, production=parserName )
        assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )

def getSuite():
    return unittest.makeSuite(PrintersTests,'test')

if __name__ == "__main__":
    unittest.main(defaultTest="getSuite")
        
