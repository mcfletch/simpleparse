""" mxTextTools - A tools package for fast text processing.

    Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
    Copyright (c) 2000-2003, eGenix.com Software GmbH; mailto:info@egenix.com
    Copyright (c) 2003-2006, Mike Fletcher; mailto:mcfletch@vrplumber.com
    See the documentation for further information on copyrights,
    or contact the author. All Rights Reserved.
"""
from .TextTools import *
from .TextTools import __version__

try:
    import copyreg
except ImportError:
    import copy_reg as copyreg


### Make the types pickleable:

# Shortcuts for pickle (reduces the pickle's length)
def _CS(definition):
    return CharSet(definition)
def _TT(definition):
    return TagTable(definition)
def _TS(match,translate,algorithm):
    return TextSearch(match,translate,algorithm)
# Needed for backward compatibility:
def _BMS(match,translate):
    return BMS(match,translate)
def _FS(match,translate):
    return FS(match,translate)

# Module init
class modinit:

    ### Register the types

    def pickle_CharSet(cs):
        return _CS,(cs.definition,)
    def pickle_TagTable(tt):
        return _TT,(tt.compiled(),)
    def pickle_TextSearch(ts):
        return _TS,(ts.match, ts.translate, ts.algorithm)
    copyreg.pickle(CharSetType,
                    pickle_CharSet,
                    _CS)
    copyreg.pickle(TagTableType,
                    pickle_TagTable,
                    _TT)
    copyreg.pickle(TextSearchType,
                    pickle_TextSearch,
                    _TS)
    if 0:
        def pickle_BMS(so):
            return _BMS,(so.match,so.translate)
        def pickle_FS(so):
            return _FS,(so.match,so.translate)
        copyreg.pickle(BMSType,
                        pickle_BMS,
                        _BMS)
        copyreg.pickle(FSType,
                        pickle_FS,
                        _FS)
        

del modinit
