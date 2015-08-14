"""Values to match result-trees even when implementations change

These values match the "logical" values of the result-trees
as they apply to SimpleParse's usage, rather than the particular
concrete results returned by the engine.  So, for instance, you
can say "returns no children" (NullResults) for result-tuples or
"whatever failure position" for failure return values.
"""

class _NullResults(object):
    def __eq__(self, other):
        return other == [] or other == None
    def __repr__( self ):
        return "<Null Children>"
NullResult = _NullResults()
class _AnyInt:
    def __eq__(self, other):
        return type(other) == type(1)
    def __repr__( self ):
        return "<Any Integer>"
AnyInt = _AnyInt()
