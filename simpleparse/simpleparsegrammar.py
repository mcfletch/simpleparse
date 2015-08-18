'''Default SimpleParse EBNF grammar as a generator with productions

This module defines the original SimpleParse
grammar.  It uses the generator objects directly
as this is the first grammar being written.
'''
from simpleparse.objectgenerator import *
from simpleparse import generator, baseparser
from simpleparse.dispatchprocessor import *

try:
    _unichr = unichr
    _unicode = unicode
except NameError:
    _unichr = chr
    _unicode = str

# note that whitespace is slightly different
# due to a bug with NULL-matching repeating groups
# we make all the ts references ts?
whitespace = Name (value = "ts", report = 0)
element_token = Name( value = "element_token" )
literal = Name ( value = "literal")
group = Name ( value = "group")
characterrange = Name ( value = "range")
name = Name ( value = "name")


SPGenerator = generator.Generator ()

SPGenerator.addDefinition(
    "declarationset",
    Name (value = "declaration", repeating = 1),
)



SPGenerator.addDefinition (
    "declaration",
    SequentialGroup (
        children = [
            whitespace,
            FirstOfGroup (
                children = [
                    Name (value = "unreportedname", ),
                    Name (value = "expandedname", ),
                    Name (value = "name", ),
                ],
            ),
            whitespace,
            Literal (value = ":"),
            Literal (value = ":", optional=1),
            Literal (value = "=",),
            Name( value = "seq_group"),
        ],
    )
)

SPGenerator.addDefinition (
    "group",
    SequentialGroup (
        children = [
            Literal (value ="("),
            Name( value= "seq_group"),
            Literal (value =")"),
        ],
        expanded = 1,
    )
)

_seq_children = FirstOfGroup(
    children = [
        Name(value="error_on_fail"),
        Name(value="fo_group"),
        Name(value="element_token"),
    ],
)

SPGenerator.addDefinition (
    "seq_group",
    SequentialGroup (
        children = [
            whitespace,
            _seq_children,
            SequentialGroup(
                children = [
                    whitespace,
                    Name( value="seq_indicator"),
                    whitespace,
                    _seq_children,
                ],
                repeating = 1, optional = 1,
            ),
            whitespace,
        ],
    ),
)

SPGenerator.addDefinition (
    "fo_group",
    SequentialGroup (
        children = [
            element_token,
            SequentialGroup(
                children = [
                    whitespace,
                    Name( value="fo_indicator"),
                    whitespace,
                    element_token,
                ],
                repeating = 1,
            ),
        ],
    )
)
SPGenerator.addDefinition (
    "seq_indicator",
    Literal(value = ",", report=0 ),
)	
SPGenerator.addDefinition (
    "fo_indicator",
    Literal(value = "/", report=0 ),
)	

SPGenerator.addDefinition (
    "element_token",
    SequentialGroup (
        children = [
            Name (value = "lookahead_indicator", optional = 1),
            whitespace,
            Name (value = "negpos_indicator", optional = 1),
            whitespace,
            FirstOfGroup (
                children = [
                    literal,
                    characterrange,
                    group,
                    name,
                ]
            ),
            whitespace,
            Name (value = "occurence_indicator", optional = 1),
            whitespace,
            Name (value = "error_on_fail", optional = 1),
        ]
    )
)

SPGenerator.addDefinition (
    "negpos_indicator",
    Range (value = "+-" )
)
SPGenerator.addDefinition (
    "lookahead_indicator",
    Literal(value = "?" ),
)	

SPGenerator.addDefinition (
    "occurence_indicator",
    Range (value = "+*?" ),
)	
SPGenerator.addDefinition (
    "error_on_fail",
    SequentialGroup (
        children = [
            Literal (value ="!"),
            SequentialGroup (
                children = [
                    whitespace,
                    Name( value="literal"),
                ],
                optional = 1,
            ),
        ],
    ),
)

SPGenerator.addDefinition (
    "unreportedname",
    SequentialGroup (
        children = [
            Literal (value ="<"),
            whitespace,
            name,
            whitespace,
            Literal (value =">"),
        ]
    )
)
SPGenerator.addDefinition (
    "expandedname",
    SequentialGroup (
        children = [
            Literal (value =">"),
            whitespace,
            name,
            whitespace,
            Literal (value ="<"),
        ]
    )
)

SPGenerator.addDefinition (
    "name",
    SequentialGroup (
        children = [
            Range(value ='abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_'),
            Range(value ='abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789', optional= 1, repeating= 1),
        ]
    )
)

SPGenerator.addDefinition (
    "ts", # ( [ \011-\015]+ / ('#',-'\n'+,'\n')+ )*
    FirstOfGroup (
        children = [
            Range(value =' \011\012\013\014\015', repeating=1),
            Name( value = "comment" ),
        ],
        repeating = 1, optional=1,
    )
)
SPGenerator.addDefinition (
    "comment", # ( [ \011-\015]+ / ('#',-'\n'+,'\n')+ )*
    SequentialGroup (
        children = [
            Literal ( value ="#"),
            Literal (value ="\n", negative = 1, repeating = 1, optional=1),
            Literal (value = "\n",),
        ],
    ),
)

SPGenerator.addDefinition (
    "literalDecorator", # literalDecorator    :=  [c]
    Range( value = 'c' )
)

SPGenerator.addDefinition (
    "literal",  # ("'",(CHARNOSNGLQUOTE/ESCAPEDCHAR)*,"'")  /  ('"',(CHARNODBLQUOTE/ESCAPEDCHAR)*,'"')
    SequentialGroup(
        children = [
            Name( value = 'literalDecorator', optional=1 ),
            FirstOfGroup (
                children = [
                    SequentialGroup (
                        children = [
                            Literal (value ="'"),
                            FirstOfGroup (
                                children = [
                                    Name (value = "CHARNOSNGLQUOTE"),
                                    Name (value = "ESCAPEDCHAR"),
                                ],
                                optional = 1, repeating = 1,
                            ),
                            Literal (value ="'"),
                        ],
                    ),
                    SequentialGroup (
                        children = [
                            Literal (value ='"'),
                            FirstOfGroup (
                                children = [
                                    Name (value = "CHARNODBLQUOTE"),
                                    Name (value = "ESCAPEDCHAR"),
                                ],
                                optional = 1, repeating = 1,
                            ),
                            Literal (value ='"'),
                        ],
                    )
                ],
            ),
        ],
    )
)

SPGenerator.addDefinition (
    "range",   # '[',CHARBRACE?,CHARDASH?, (CHARRANGE/CHARNOBRACE)*, CHARDASH?,']'
    SequentialGroup (
        children =[
            Literal (value ="["),
            Name (value ="CHARBRACE",optional = 1),
            Name (value ="CHARDASH",optional = 1),
            FirstOfGroup(
                children = [
                    Name (value ="CHARRANGE"),
                    Name (value ="CHARNOBRACE"),
                ],
                optional = 1, repeating = 1,
            ),
            Name (value ="CHARDASH",optional = 1),
            Literal (value ="]"),
        ],
    )
)
SPGenerator.addDefinition (
    "CHARBRACE",   
    Literal (value = "]"),
)
SPGenerator.addDefinition (
    "CHARDASH",   
    Literal (value = "-"),
)
SPGenerator.addDefinition (
    "CHARRANGE",   # CHARRANGE           :=  CHARNOBRACE, '-', CHARNOBRACE
    SequentialGroup (
        children =[
            Name (value ="CHARNOBRACE"),
            Literal (value ="-"),
            Name (value ="CHARNOBRACE"),
        ],
    ),
)
SPGenerator.addDefinition (
    "CHARNOBRACE",   # CHARRANGE           :=  CHARNOBRACE, '-', CHARNOBRACE
    FirstOfGroup(
        children =[
            Name (value ="ESCAPEDCHAR"),
            Name (value ="CHAR"),
        ],
    ),
)
SPGenerator.addDefinition (
    "CHAR",
    Literal (
        value ="]",
        negative = 1,
    ),
)

SPGenerator.addDefinition (
    "ESCAPEDCHAR",   # '\\',( SPECIALESCAPEDCHAR / ('x',HEXESCAPEDCHAR) / UNICODEESCAPEDCHAR_16 / OCTALESCAPEDCHAR /   )
    SequentialGroup (
        children =[
            Literal (value ="\\"),
            FirstOfGroup(
                children = [
                    Name (value ="SPECIALESCAPEDCHAR"),
                    SequentialGroup(
                        children = [
                            Range( value = 'xX' ),
                            Name( value="HEXESCAPEDCHAR"),
                        ]
                    ),
                    Name (value ="OCTALESCAPEDCHAR"),
                    SequentialGroup(
                        children = [
                            Range( value='uU'),
                            Name( value='UNICODEESCAPEDCHAR' ),
                        ],
                    ),
                ],
            ),
        ],
    )
)

SPGenerator.addDefinition (
    "SPECIALESCAPEDCHAR",
    Range(value ='\\abfnrtv"\''),
)

SPGenerator.addDefinition (
    "OCTALESCAPEDCHAR",   # [0-7],[0-7]?,[0-7]?
    SequentialGroup (
        children =[
            Range (value ="01234567"),
            Range (value ="01234567", optional = 1),
            Range (value ="01234567", optional = 1),
        ],
    )
)
SPGenerator.addDefinition (
    "HEXESCAPEDCHAR",   # [0-9a-fA-F],[0-9a-fA-F]
    SequentialGroup (
        children =[
            Range (value ="0123456789abcdefABCDEF"),
            Range (value ="0123456789abcdefABCDEF"),
        ],
    )
)
SPGenerator.addDefinition(
    "UNICODEESCAPEDCHAR",
    SequentialGroup(
        children=[
            Range (value ="0123456789abcdefABCDEF"),
            Range (value ="0123456789abcdefABCDEF"),
            Range (value ="0123456789abcdefABCDEF"),
            Range (value ="0123456789abcdefABCDEF"),
            SequentialGroup(
                children = [
                    Range (value ="0123456789abcdefABCDEF"),
                    Range (value ="0123456789abcdefABCDEF"),
                    Range (value ="0123456789abcdefABCDEF"),
                    Range (value ="0123456789abcdefABCDEF"),
                ],
                optional = True,
            )
        ]
    )
)

SPGenerator.addDefinition (
    "CHARNODBLQUOTE",
    Range(value ='\\"', negative = 1, repeating = 1),
)
SPGenerator.addDefinition (
    "CHARNOSNGLQUOTE",
    Range(value ="\\'", negative = 1, repeating = 1),
)

declaration = r"""declarationset      :=  declaration+
declaration         :=  ts, (unreportedname/expandedname/name) ,ts,':',':'?,'=',seq_group

element_token       :=  lookahead_indicator?, ts, negpos_indicator?,ts, (literal/range/group/name),ts, occurence_indicator?, ts, error_on_fail?

negpos_indicator    :=  [-+]
lookahead_indicator :=  "?"
occurence_indicator :=  [+*?]
error_on_fail       :=  "!", (ts,literal)?

>group<             :=  '(',seq_group, ')'
seq_group           :=  ts,(error_on_fail/fo_group/element_token),
                          (ts, seq_indicator, ts,
                              (error_on_fail/fo_group/element_token)
                          )*, ts

fo_group            :=  element_token, (ts, fo_indicator, ts, element_token)+


# following two are likely something peoples might want to
# replace in many instances...
<fo_indicator>      :=  "/"
<seq_indicator>     :=  ','

unreportedname      :=  '<', name, '>'
expandedname        :=  '>', name, '<'
name                :=  [a-zA-Z_],[a-zA-Z0-9_]*
<ts>                :=  ( [ \011-\015]+ / comment )*
comment             :=  '#',-'\n'*,'\n'
literal             :=  literalDecorator?,("'",(CHARNOSNGLQUOTE/ESCAPEDCHAR)*,"'")  /  ('"',(CHARNODBLQUOTE/ESCAPEDCHAR)*,'"')
literalDecorator    :=  [c]



range               :=  '[',CHARBRACE?,CHARDASH?, (CHARRANGE/CHARNOBRACE)*, CHARDASH?,']'
CHARBRACE           :=  ']'
CHARDASH            :=  '-'
CHARRANGE           :=  CHARNOBRACE, '-', CHARNOBRACE
CHARNOBRACE         :=  ESCAPEDCHAR/CHAR
CHAR                :=  -[]]
ESCAPEDCHAR         :=  '\\',( SPECIALESCAPEDCHAR / ('x',HEXESCAPEDCHAR) / ([uU],UNICODEESCAPEDCHAR) / OCTALESCAPEDCHAR  )
SPECIALESCAPEDCHAR  :=  [\\abfnrtv"']
OCTALESCAPEDCHAR    :=  [0-7],[0-7]?,[0-7]?
HEXESCAPEDCHAR      :=  [0-9a-fA-F],[0-9a-fA-F]
CHARNODBLQUOTE      :=  -[\\"]+
CHARNOSNGLQUOTE     :=  -[\\']+
UNICODEESCAPEDCHAR  := [0-9a-fA-F],[0-9a-fA-F],[0-9a-fA-F],[0-9a-fA-F],([0-9a-fA-F],[0-9a-fA-F],[0-9a-fA-F],[0-9a-fA-F])?
"""

### Now the interpreter objects...
class Parser(baseparser.BaseParser):
    """Parser which generates new parsers from EBNF grammars

    This parser class allows you to pass in an EBNF grammar as
    the initialisation parameter.  The EBNF is processed, and a
    SimpleParse generator object is created as self.generator.

    Unlike most Parsers, this object is intended to be re-created
    for each bit of data it parses (i.e. each EBNF), so it warps
    the standard API a lot.
    """
    _rootProduction = 'declarationset'
    def __init__( self, ebnf, prebuilts=(), methodSource=None, definitionSources=() ):
        """Create a new generator based on the EBNF in simpleparse format"""
        processor = SPGrammarProcessor( prebuilts, definitionSources )
        success, tags, next = self.parse( ebnf, self._rootProduction, processor=processor )
        if next != len(ebnf):
            lineNumber = lines(0, next, ebnf)
            raise ValueError(
                """Unable to complete parsing of the EBNF, stopped at line %s (%s chars of %s)
Unparsed:\n%s..."""%(lineNumber, next, len(ebnf), ebnf[next:next+100])
            )
        self.generator = processor.generator
    def buildTagger( self, name=None, processor = None ):
        """Build the tag-table for parsing the EBNF for this parser"""
        return SPGenerator.buildParser( name, processor )

class SPGrammarProcessor( DispatchProcessor ):
    """Processing object for post-processing an EBNF into a new generator"""
    ### top level
    def __init__( self, prebuilts=(), definitionSources=() ):
        """Create a new generator based on the EBNF in simpleparse format"""
        self.generator = generator.Generator()
        for (name, table) in prebuilts:
            if isinstance( table, ElementToken):
                self.generator.addDefinition( name, table)
            else:
                self.generator.addDefinition( name, Prebuilt(value=table))
        for source in definitionSources:
            self.generator.addDefinitionSource( source )
    
    def declaration( self, info, buffer):
        '''Base declaration from the grammar, a "production" or "rule"'''
        (tag, left, right, sublist) = info
        name = sublist[0]
        expanded = 0
        if name[0] == "unreportedname":
            name = name[3][0]
            # note that the info is stored in the wrong place :(
            report = 0
        elif name[0] == 'expandedname':
            report = 1
            expanded = 1
            name = name[3][0]
        else:
            report = 1
        name = getString( name, buffer )
        self.currentProduction = name
        content = dispatch( self, sublist[1], buffer )
        content.report = report
        content.expanded = expanded
        self.generator.addDefinition(
            name,
            content,
        )
        del self.currentProduction

    ### element configuration
    def element_token( self, info, buffer):
        '''get the children, then configure'''
        (tag, left, right, sublist) = info
        base = None
        negative = 0
        optional = 0
        repeating = 0
        lookahead = 0
        errorOnFail = None
        for tup in sublist:
            result = dispatch( self, tup, buffer )
            if tup[0] == 'negpos_indicator':
                negative = result
            elif tup[0] == 'occurence_indicator':
                optional, repeating = result
            elif tup[0] == 'lookahead_indicator':
                lookahead = result
            elif tup[0] == 'error_on_fail':
                # we do some extra work here
                errorOnFail = result
                self._config_error_on_fail( errorOnFail, (tag,left,tup[1],[]), buffer )
            else:
                base = result
        base.optional = optional
        base.negative = negative
        base.repeating = repeating
        base.lookahead = lookahead
        if errorOnFail:
            base.errorOnFail = errorOnFail
        return base

    ### generator-node-builders
    def seq_group( self, info, buffer):
        """Process a sequential-group into a SequentialGroup element token"""
        (tag, left, right, sublist) = info
        children = dispatchList( self, sublist, buffer )
        errorOnFail = None
        result = []
        for (item,tup) in zip(children,sublist):
            if isinstance( item, ErrorOnFail ):
                errorOnFail = item
            else:
                if errorOnFail:
                    item.errorOnFail = errorOnFail.copy()
                    self._config_error_on_fail(
                        item.errorOnFail,
                        tup,
                        buffer
                    )
                result.append( item )
        if len(result) == 1:
            # single-item sequential group (very common)
            return result[0]
        elif not result:
            raise ValueError( """SequentialGroup on line %s doesn't have an element-token child! grammar was %s"""%( lines(0,left, buffer), buffer[left:left+25]))
        base = SequentialGroup(
            children = result,
        )
        return base
    def fo_group( self, info, buffer):
        """Process a first-of-group into a FirstOf element token"""
        (tag, left, right, sublist) = info
        children = dispatchList( self, sublist, buffer )
        if len(children) == 1:
            # this should never happen, but if it does, we can deal with it I suppose...
            return children[0]
        base = FirstOfGroup(
            children = children
        )
        return base
        
    def literal( self, info, buffer):
        '''Turn a literal result into a literal generator'''
        (tag, left, right, sublist) = info
        if sublist and sublist[0][0] == 'literalDecorator':
            # right now only have the one decorator...
            sublist = sublist[1:]
            classObject = CILiteral
        else:
            classObject = Literal
        elements = dispatchList( self, sublist, buffer)
        ### Should check for CILiteral with non-CI string or single-character value!
        return classObject( value = "".join(elements) )

    def range( self, info, buffer):
##		if hasattr( Range, 'requiresExpandedSet') and Range.requiresExpandedSet:
        (tag, left, right, sublist) = info
        return Range(
            value = ''.join(dispatchList( self, sublist, buffer)),
        )
##		else:
##			# need to build up a new-syntax version of the range...
##			# escape ^ to \^
##			# escape \ to \\
##			# escape - to \-
##			# make sure range-sets are in proper order...
##			raise NotImplementedError( """Haven't got the new CharSet version implemented yet""")
    def name( self, tup, buffer):
        return Name(
            value = getString(tup, buffer),
        )
    ### simple translators
    occurenceIndicatorMap = {
        '*': (1,1),
        '+': (0,1),
        '?': (1,0),
    }
    def occurence_indicator( self, tup, buffer):
        '''Return optional, repeating as a tuple of true/false values'''
        value = getString(tup, buffer)
        return self.occurenceIndicatorMap[value]
    def lookahead_indicator( self, tup, buffer ):
        """If present, the lookahead indictor just says "yes", so just return 1"""
        return 1
    def error_on_fail( self, info, buffer ):
        """If present, we are going to make the current object an errorOnFail type,

        If there's a string literal child, then we use it to create the
        "message" attribute of the errorOnFail object.
        """
        (tag,left,right,children) = info
        err = ErrorOnFail()
        if children:
            (tag,left,right,children) = children[0]
            message = "".join(dispatchList( self, children, buffer))
            err.message = message
        return err
    def _config_error_on_fail( self, errorOnFail, tup, buffer ):
        """Configure an error-on-fail instance for a given child tuple"""
        # what we expected to find...
        errorOnFail.expected = buffer[tup[1]:tup[2]]
        if hasattr( self, "currentProduction"):
            errorOnFail.production = self.currentProduction
        

    negposIndicatorMap = {
        '+': 0,
        '-': 1,
    }
    def negpos_indicator( self, tup, buffer ):
        '''return whether indicates negative'''
        value = getString(tup, buffer)
        return self.negposIndicatorMap[value]

    def CHARNODBLQUOTE( self, tup, buffer):
        return getString(tup, buffer)
    CHAR = CHARNOSNGLQUOTE = CHARNODBLQUOTE
    def ESCAPEDCHAR( self, info, buffer):
        (tag, left, right, sublist) = info
        return "".join(dispatchList( self, sublist, buffer))
    specialescapedmap = {
    'a':'\a',
    'b':'\b',
    'f':'\f',
    'n':'\n',
    'r':'\r',
    't':'\t',
    'v':'\v',
    '\\':'\\',
    '"':'"',
    "'":"'",
    }
    def SPECIALESCAPEDCHAR( self, tup, buffer):
        return self.specialescapedmap[ getString(tup, buffer)]
    def OCTALESCAPEDCHAR(self, tup, buffer):
        return chr(int( getString(tup, buffer), 8 ))
    def HEXESCAPEDCHAR( self, tup , buffer):
        return chr(int( getString(tup, buffer), 16 ))
    def CHARNOBRACE( self, info, buffer):
        (tag, left, right, sublist) = info
        return "".join(dispatchList( self, sublist, buffer))
    def CHARRANGE( self, info, buffer):
        '''Create a string from first to second item'''
        (tag, left, right, sublist) = info
        first,second = dispatchList( self, sublist, buffer)
        if second < first:
            second, first = first, second
        if isinstance( first, _unicode ) or isinstance( second, _unicode ):
            _chr = _unichr
            if not (isinstance( second, _unicode ) and isinstance( first, _unicode )):
                raise ValueError( 'Range %s uses one unicode and one string escape, cannot mix'%(buffer[left:right]) )
        else:
            _chr = chr 
        first, second = list(map( ord, (first,second) ))
        return u''.join([_chr(u) for u in range(first,second+1)])
    def CHARDASH( self, tup , buffer):
        return '-'
    def CHARBRACE( self, tup , buffer):
        return ']'

    def UNICODEESCAPEDCHAR( self, info, buffer):
        """Decode a unicode-escaped hex character into a character value"""
        (tag, left, right, sublist) = info
        char = _unichr(int( buffer[left:right], 16 ))
        return char
