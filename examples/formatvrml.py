"""Example using a parser to format VRML97 code as HTML w/CSS
"""
import sys
import os, string
from simpleparse.parser import Parser

# we use a non-standard VRML parser definition which retains comments and whitespace
VRMLPARSERDEF = r'''
# Specialised VRML parser for colourising VRML files
# Specialisation is minor, mostly changes to what information
# is reported and what information is discarded.

vrmlFile           := '#',header,'\n',ts, vrmlScene, ts
header                 := -[\n]*
vrmlScene          := (Proto/ExternProto/ROUTE/('USE',ts,USE,ts)/Script/Node/SFNull)*
Proto                  := PROTO,ts,nodegi,ts,'[',ts,(fieldDecl/eventDecl)*,']', ts, '{', ts, vrmlScene,ts, '}', ts
fieldDecl	         := fieldExposure,ts,dataType,ts,attrName,ts,Field,ts
fieldExposure  := 'field'/'exposedField'
dataType           := 'SFBool'/'SFString'/'SFFloat'/'SFTime'/'SFVec3f'/'SFVec2f'/'SFRotation'/'SFInt32'/'SFImage'/'SFRotation'/'SFColor'/'SFNode'/'MFBool'/'MFString'/'MFFloat'/'MFTime'/'MFVec3f'/'MFVec2f'/'MFRotation'/'MFInt32'/'MFRotation'/'MFColor'/'MFNode'
eventDecl          := eventDirection, ts, dataType, ts, eventName, ts
eventDirection := 'eventIn'/'eventOut'
ExternProto        := EXTERNPROTO,ts,nodegi,ts,'[',ts,(extFieldDecl/eventDecl)*,']', ts, ExtProtoURL
extFieldDecl   := fieldExposure,ts,dataType,ts,name,ts
ExtProtoURL        := '['?,ts,SFString+, ']'?, ts  # just an MFString by another name :)
ROUTEData          := 'ROUTE',ts, DEFName,'.',DEFName, ts, 'TO', ts, DEFName,'.',DEFName
ROUTE                  := ROUTEData, ts
Node                   := (DEF,ts,DEFName,ts)?,nodegi,ts,'{',ts,(Proto/ExternProto/ROUTE/Attr)*,ts,'}', ts
Script                 := (DEF,ts,DEFName,ts)?,scriptgi,ts,'{',ts,(ScriptFieldDecl/ScriptEventDecl/Proto/ExternProto/ROUTE/Attr)*,ts,'}', ts
ScriptEventDecl := eventDirection, ts, dataType, ts, attrName, ts, ('IS', ts, IS,ts)?
ScriptFieldDecl := fieldExposure,ts,dataType,ts,attrName,ts,(('IS', ts,IS,ts)/Field),ts
SFNull                 := 'NULL', ts

EXTERNPROTO        := 'EXTERNPROTO'
PROTO                  := 'PROTO'
scriptgi           := 'Script'
DEF                        := 'DEF'
eventName          := name
DEFName                := name
USE                        := name
IS                         := name
nodegi                 := name 
attrName           := name
Attr                   := attrName, ts, (('IS', ts,IS,ts)/Field), ts
Field                  := ( '[',ts,((SFNumber/SFBool/SFString/('USE',ts,USE,ts)/Script/Node),ts)*, ']', ts )/((SFNumber/SFBool/SFNull/SFString/('USE',ts,USE,ts)/Script/Node),ts)+

<name>                   := -[][0-9{}\000-\020"'#,.\\ ],  -[][{}\000-\020"'#,.\\ ]*
<SFNumber>           := [-+0-9.]+,([eE],[-+0-9.]+)?
<SFBool>                 := 'TRUE'/'FALSE'
SFString                 := '"',(CHARNODBLQUOTE/ESCAPEDCHAR)*,'"'
<CHARNODBLQUOTE> :=  -[\134"]+
<ESCAPEDCHAR>        := '\\"'/'\134\134'
comment                  := '#',-'\012'*,'\n'
ts                           :=  ( [ \011-\015,]+ / comment+ )*
'''
vrmlparser = Parser( VRMLPARSERDEF, 'vrmlFile' )

class VRMLFormatter:
	'''
	Base formatting class
	'''
	def __init__(self, infile, vrmlparser = vrmlparser ):
		self.infile = open( infile ).read()
		self.tree = vrmlparser.parse( self.infile )[1] # the list of children
		# construct a dummy "vrmlFile" node, should get that fixed in TextTools some day
		self.tree = ('vrmlFile', 0, len(self.infile), self.tree )
	
	def _format( self, tup, outfile, infile ):
		'''
		Step through the children, our result is
		thisnode's head, data_to_first_child, firstchild, data_to_second_child, secondchild,...,data_from_last_child, thisnode's tail
		'''
		nodetype = tup[0]
		# write preceding formatting
		hdata = self._headdata( nodetype, 1 )
		if hdata is not None:
			outfile.write( hdata )
		startPos = tup[1]
		children = tup[3][:]
		while children:
			outfile.write( self._escapeData( infile[ startPos: children[0][1] ] )  )
			self._format( children[0], outfile, infile )
			startPos = children[0][2]
			del children [0]
		# now write this node's data from startPos to endPos
		outfile.write( self._escapeData( infile[startPos: tup[2] ]) )
		# write trailing formatting
		hdata = self._headdata( nodetype, 0 )
		if hdata is not None:
			outfile.write( hdata )
		
	def _headdata( self, nodetype, head=1 ):
		'''
		Return head or tail data for this nodetype if available, None otherwise
		'''
		if head:
			head = '_head'
		else:
			head = '_tail'
		if hasattr( self, nodetype+head ):
			return getattr( self, nodetype+head) % locals()
	def _escapeData( self, data ):
		return data
		
	def format( self, outfile ):
		outfile = open( outfile, 'w' )
		self._format( self.tree, outfile, self.infile )
		outfile.close()

class HTMLVRMLFormatter( VRMLFormatter ):
	'''
	Format VRML files for display in HTML
	'''
	def _escapeData( self, data ):
		return string.join( string.split( 
			string.join( string.split( 
				string.join( string.split( 
					string.join( string.split( data, '&' ), '&amp;' ),
				'<'), '&lt;'),
			'>'), '&gt;'),
		'\t'), '  ')
			
		
	NODEMAP = {
		'vrmlFile': '''<html><head><link href="vrmlCode.css" rel="stylesheet" type="text/css"></head><body><pre>''',
		'vrmlFile_tail':'''\n</pre></body></html>''',
		'header':'<%(head)sfont color="purple">',
		'header_tail':'<%(head)sfont>',
		'comment':'<span class="%(nodetype)s">',
		'comment_tail':'</span>',
		'PROTO':'<span class="%(nodetype)s">',
		'PROTO_tail':'</span>',
		'EXTERNPROTO':'<span class="%(nodetype)s">',
		'EXTERNPROTO_tail':'</span>',
		'SFString':'<span class="%(nodetype)s">',
		'SFString_tail':'</span>',
		
		
		'DEF':'<%(head)sstrong>',
#                'name':'<%(head)sfont color="green">',
#                'name_tail':'<%(head)sfont>',
		'DEFName':'<span class="%(nodetype)s">',
		'DEFName_tail':'</span>',
		'nodegi':'<span class="%(nodetype)s">',
		'nodegi_tail':'</span>',
		'scriptgi':'<span class="%(nodetype)s">',
		'scriptgi_tail':'</span>',
		'ROUTEData':'<strong class="%(nodetype)s">',
		'ROUTEData_tail':'</span>',
		'attrName':'<span class="%(nodetype)s">',
		'attrName_tail':'</span>',
		'fieldExposure':'<span class="%(nodetype)s">',
		'fieldExposure_tail':'</span>',
		'dataType':'<span class="%(nodetype)s">',
		'dataType_tail':'</span>',
		'eventDirection':'<span class="%(nodetype)s">',
		'eventDirection_tail':'</span>',
		
		
	}
	def _headdata( self, nodetype, head=1):
		if head:
			head = ''
			return self.NODEMAP.get( nodetype, '' )%locals()
		else:
			head = '/'
			val = self.NODEMAP.get( nodetype+'_tail', '' )%locals()
			if not val:
				return self.NODEMAP.get( nodetype, '' )%locals()
			else:
				return val

usage = '''formatvrml.py infile outfile
	infile -- properly formatted VRML 97 file
	outfile -- destination for output HTML (will overwrite if present)

Description:
	Formatvrml is a simple script for syntax-coloring VRML 97 code for
	presentation on web sites and/or in documentation.  To use it, just
	run the script with your source and destination files.  Copy the
	HTML and the css file to your web server.

	The syntax coloring is all done with a Cascading Style Sheet link
	at the top of the file (to a file named vrmlCode.css in the same
	directory as the HTML file).  You can change the formatting of your
	VRML by changing this file's definitions.
'''

if __name__ == '__main__':
	import sys
	if len( sys.argv) != 3:
		print usage
		raw_input('Press <return> to exit:')
	else:
		file = HTMLVRMLFormatter( sys.argv[1] )
		file.format( sys.argv[2] )
