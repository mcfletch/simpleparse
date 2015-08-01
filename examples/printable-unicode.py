#!/usr/bin/env python
# -*- coding: utf-8 -*-

declaration = r'''
printable_inline ::= printable_inline_char+
printable_inline_char ::= '\x09' / [\u0020-\uD7FF] / [\uE000-\uFFFD] / [\u00010000-\u00010FFF]
'''

testdata = u'''\t ß ‧  æ č ☭  ° 漢字 —  '''
from simpleparse.parser import Parser
import pprint

parser = Parser( declaration, "printable_inline" )

if __name__ =="__main__":
    pprint.pprint( parser.parse(testdata))
    
