"""VRML97-compliant Parser

This example is a full VRML97 parser, originally created
for the mcf.vrml VRML-processing system.  It supports all
VRML97 constructs, and should be correct for any VRML97
content you can produce.  The parser is fairly fast
(parsing around 280,000 cps on a 1GHz Athlon machine).

This is the errorOnFail version of the grammar, otherwise
identical to the vrml.py module.  Note: there is basically
no speed penalty for the errorOnFail version compared to
the original version, as the errorOnFail code is not touched
unless a syntax error is actually found in the input text.
"""
from simpleparse.parser import Parser
from simpleparse.common import chartypes

#print file
VRMLPARSERDEF = r'''header         := -[\n]*
vrmlFile       := header, vrmlScene, EOF
rootItem       := ts,(Proto/ExternProto/ROUTE/('USE',ts,USE,ts)/Script/Node),ts
vrmlScene      := rootItem*

Proto          := 'PROTO',ts,!, nodegi,ts,'[',ts,(fieldDecl/eventDecl)*,']', ts, '{', ts, vrmlScene,ts, '}', ts
fieldDecl	   := fieldExposure,ts,!,dataType,ts,name,ts,Field,ts
fieldExposure  := 'field'/'exposedField'
dataType       := 'SFBool'/'SFString'/'SFFloat'/'SFTime'/'SFVec3f'/'SFVec2f'/'SFRotation'/'SFInt32'/'SFImage'/'SFColor'/'SFNode'/'MFBool'/'MFString'/'MFFloat'/'MFTime'/'MFVec3f'/'MFVec2f'/'MFRotation'/'MFInt32'/'MFColor'/'MFNode'
eventDecl      := eventDirection, ts, !,dataType, ts, name, ts
eventDirection := 'eventIn'/'eventOut'
ExternProto    := 'EXTERNPROTO',ts,!,nodegi,ts,'[',ts,(extFieldDecl/eventDecl)*,']', ts, ExtProtoURL
extFieldDecl   := fieldExposure,ts,!,dataType,ts,name,ts
ExtProtoURL    := '['?,(ts,SFString)*, ts, ']'?, ts  # just an MFString by another name :)

ROUTE          := 'ROUTE',ts, !,name,'.',name, ts, 'TO', ts, name,'.',name, ts

Node           := ('DEF',ts,!,name,ts)?,nodegi,ts,'{',ts,(Proto/ExternProto/ROUTE/Attr)*,ts,!,'}', ts

Script         := ('DEF',ts,!,name,ts)?,'Script',ts,!,'{',ts,(ScriptFieldDecl/ScriptEventDecl/Proto/ExternProto/ROUTE/Attr)*,ts,'}', ts
ScriptEventDecl := eventDirection, ts, !,dataType, ts, name, ts, ('IS', ts,!, IS,ts)?
ScriptFieldDecl := fieldExposure,ts,!,dataType,ts,name,ts,(('IS', ts,!,IS,ts)/Field),ts

SFNull         := 'NULL', ts

# should really have an optimised way of declaring a different reporting name for the same production...
USE            := name
IS             := name
nodegi         := name 
Attr           := name, ts, (('IS', ts,IS,ts)/Field), ts
Field          := ( '[',ts,((SFNumber/SFBool/SFString/('USE',ts,USE,ts)/Script/Node),ts)*, ']'!, ts )/((SFNumber/SFBool/SFNull/SFString/('USE',ts,USE,ts)/Script/Node),ts)+

name           := -[][0-9{}\000-\020"'#,.\\ ],  -[][{}\000-\020"'#,.\\ ]*
SFNumber       := [-+]*, ( ('0',[xX],[0-9]+) / ([0-9.]+,([eE],[-+0-9.]+)?))
SFBool         := 'TRUE'/'FALSE'
SFString       := '"',(CHARNODBLQUOTE/ESCAPEDCHAR/SIMPLEBACKSLASH)*,'"'!
CHARNODBLQUOTE :=  -[\134"]+
SIMPLEBACKSLASH := '\134'
ESCAPEDCHAR    := '\\"'/'\134\134'
<ts>           :=  ( [ \011-\015,]+ / ('#',-'\012'*,'\n')+ )*
'''

def buildVRMLParser( declaration = VRMLPARSERDEF ):
	return Parser( declaration, "vrmlFile" )

if __name__ == "__main__":
	import os, sys, time
	parser = buildVRMLParser()
	if sys.argv[1:]:
		filename = sys.argv[1]
		data = open(filename).read()
		t = time.time()
		success, tags, next = parser.parse( data)
		d = time.time()-t
		print "parsed %s characters of %s in %s seconds (%scps)"%( next, len(data), d, next/(d or 0.000000001) )
	# now show the error-generation
	print '''About to parse badly formatted VRML data'''
	badData = [
		'''#whatever\nX{ { } }''',
		'''#whatever\nX{ S }''',
		'''#whatever\nPROTO ]{ S }''',
		'''#whatever\nPROTO []{ S ''',
		'''#whatever\nPROTO R [
		field SFBool A
]{   }''',
		'''#whatever\nPROTO R [
		field SFBool
]{   }''',
		'''#whatever\nPROTO R [
		field SFBool A "
]{   ''',
	]
		
	for bad in badData:
		try:
			parser.parse( bad )
			print """\nWARNING: didn't get a syntax error for item %s\n"""%(repr(bad))
		except SyntaxError, err:
			print err
