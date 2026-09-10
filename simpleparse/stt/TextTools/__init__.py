""" mxTextTools - A tools package for fast text processing.

    Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
    Copyright (c) 2000-2003, eGenix.com Software GmbH; mailto:info@egenix.com
    Copyright (c) 2003-2006, Mike Fletcher; mailto:mcfletch@vrplumber.com
    See the documentation for further information on copyrights,
    or contact the author. All Rights Reserved.
"""
from .TextTools import *
from .TextTools import __version__

import copyreg


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

# Register the types with the pickler, so a compiled set, table or search
# survives being pickled with whatever holds it.  At module level: this was a
# `class modinit:` whose body ran at import, which is a way of writing module
# init that predates everything else here and made each function below look
# like a method of it.

def pickle_CharSet(cs):
    return _CS, (cs.definition,)


def pickle_TagTable(tt):
    return _TT, (tt.compiled(),)


def pickle_TextSearch(ts):
    return _TS, (ts.match, ts.translate, ts.algorithm)


copyreg.pickle(CharSetType, pickle_CharSet, _CS)
copyreg.pickle(TagTableType, pickle_TagTable, _TT)
# copyreg declares the constructor as taking the reduce tuple; this one
# takes the three values in it, which is what `_TS`'s own signature says.
copyreg.pickle(TextSearchType, pickle_TextSearch, _TS)  # type: ignore[arg-type]
