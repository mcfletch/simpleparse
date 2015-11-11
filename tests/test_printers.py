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
testModuleFile = 'test_printers_garbage.py'
HERE = os.path.dirname(__file__)
TEST_DIR = os.path.join( HERE, 'tempmodules' )

def setUp(self):
    if os.path.exists(TEST_DIR):
        shutil.rmtree( TEST_DIR )
    os.makedirs(TEST_DIR)
    open(
        os.path.join(TEST_DIR,'__init__.py'),
        'w'
    ).close()
def tearDown(self):
    shutil.rmtree( TEST_DIR )
    

class PrintersTests(test_grammarparser.SimpleParseGrammarTests):
    def setUp( self ):
        from simpleparse import simpleparsegrammar, parser, printers, baseparser
        name = self.id().split('.')[-1]
        filename = name + '.py'
        
        testModuleFile = os.path.join(TEST_DIR,filename)
        
        p = parser.Parser( simpleparsegrammar.declaration, 'declarationset')
        with open(testModuleFile,'w') as fh:
            fh.write(printers.asGenerator( p._generator ))
        mod_name = '%s.tempmodules.%s'%(__name__.rsplit('.',1)[0],name,)
        test_printers_garbage = __import__( mod_name,{},{},mod_name.split('.') )
        reload( test_printers_garbage )
        
        class RParser( test_printers_garbage.Parser, baseparser.BaseParser ):
            pass

        self.recursiveParser = RParser()
    def tearDown( self ):
        try:
            os.remove( testModuleFile )
        except (OSError,IOError):
            pass
    def doBasicTest(self, parserName, testValue, expected, ):
        result = self.recursiveParser.parse( testValue, production=parserName )
        assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )

def getSuite():
    return unittest.makeSuite(PrintersTests,'test')

if __name__ == "__main__":
    unittest.main(defaultTest="getSuite")
        
