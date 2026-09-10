"""What the mxTextTools C extension exports.

The tagging engine and the byte-level string tools the parser is built on.
Everything here is implemented in ``mxTextTools.c``; the declarations are here
so that a checker reading a program built on SimpleParse -- or SimpleParse's
own modules, which reach these through a star import -- can see what it holds.

Signatures follow the extension's own docstrings. The text these work on is
``bytes``: the tagging engine matches byte ranges and character sets, and a
caller with text encodes it first (``simpleparse.parser`` does).
"""

from typing import Any, Dict, List, Optional, Sequence, Tuple, Union

#: Which search algorithm `TextSearch` uses. `BOYERMOORE_MODERN` is the
#: default; the others are kept because a table may name one.
BOYERMOORE: int
BOYERMOORE_MODERN: int
FASTSEARCH: int
TRIVIAL: int

#: Byte translation tables, 256 bytes each, for the case-folding searches.
to_lower: bytes
to_upper: bytes

#: Tag tables already compiled, keyed by the definition they came from.
tagtable_cache: Dict[Any, Any]

class Error(Exception):
    """Raised for a malformed tag table or a command it cannot run."""

class CharSetType:
    """A compiled character set; see :func:`CharSet`."""

    #: What it was built from, which is what pickling it stores.
    definition: Union[str, bytes]

    def contains(self, character: bytes) -> int: ...
    def search(self, text: bytes, direction: int = ..., start: int = ...,
               stop: int = ...) -> Optional[int]: ...
    def match(self, text: bytes, direction: int = ..., start: int = ...,
              stop: int = ...) -> int: ...
    def split(self, text: bytes, start: int = ...,
              stop: int = ...) -> List[bytes]: ...
    def splitx(self, text: bytes, start: int = ...,
               stop: int = ...) -> List[bytes]: ...
    def strip(self, text: bytes, mode: int = ..., start: int = ...,
              stop: int = ...) -> bytes: ...

class TagTableType:
    """A compiled tag table; see :func:`TagTable`."""

    def compiled(self) -> Tuple[Any, ...]: ...

class TextSearchType:
    """A compiled search for one string; see :func:`TextSearch`."""

    #: What it searches for, and how -- the three a pickle stores.
    match: Union[str, bytes]
    translate: Optional[bytes]
    algorithm: int

    def search(self, text: bytes, start: int = ...,
               stop: int = ...) -> Tuple[int, int]: ...
    def find(self, text: bytes, start: int = ...,
             stop: int = ...) -> int: ...
    def findall(self, text: bytes, start: int = ...,
                stop: int = ...) -> List[Tuple[int, int]]: ...

#: Takes the set either way: `'a-z'` as well as `b'a-z'`.
def CharSet(definition: Union[str, bytes]) -> CharSetType: ...
def TagTable(definition: Sequence[Any],
             cachable: int = ...) -> TagTableType: ...
def UnicodeTagTable(definition: Sequence[Any],
                    cachable: int = ...) -> TagTableType: ...
#: Takes what it searches for either way: `'needle'` as well as `b'needle'`.
def TextSearch(match: Union[str, bytes], translate: Optional[bytes] = ...,
               algorithm: int = ...) -> TextSearchType: ...

def tag(text: bytes, tagtable: Any, sliceleft: int = ..., sliceright: int = ...,
        taglist: Optional[List[Any]] = ..., context: Any = ...,
        encoding: Any = ...) -> Tuple[int, List[Any], int]:
    """Run a tag table over text: ``(matched, taglist, position)``."""

def join(joinlist: Sequence[Any], sep: bytes = ..., start: int = ...,
         stop: int = ...) -> bytes: ...
def joinlist(text: bytes, list: Sequence[Any], start: int = ...,
             stop: int = ...) -> List[Any]: ...
def set(string: bytes, logic: int = ...) -> bytes: ...
def setfind(text: bytes, set: bytes, start: int = ...,
            stop: int = ...) -> Optional[int]: ...
def setsplit(text: bytes, set: bytes, start: int = ...,
             stop: int = ...) -> List[bytes]: ...
def setsplitx(text: bytes, set: bytes, start: int = ...,
              stop: int = ...) -> List[bytes]: ...
def setstrip(text: bytes, set: bytes, start: int = ..., stop: int = ...,
             mode: int = ...) -> bytes: ...
def splitat(text: bytes, char: bytes, nth: int = ..., start: int = ...,
            stop: int = ...) -> Tuple[bytes, bytes]: ...
def charsplit(text: bytes, char: bytes, start: int = ...,
              stop: int = ...) -> List[bytes]: ...
def prefix(text: bytes, prefixes: Sequence[bytes], start: int = ...,
           stop: int = ..., translate: bytes = ...) -> Optional[bytes]: ...
def suffix(text: bytes, suffixes: Sequence[bytes], start: int = ...,
           stop: int = ..., translate: bytes = ...) -> Optional[bytes]: ...
def cmp(a: Any, b: Any) -> int: ...
def hex2str(text: bytes) -> bytes: ...
def str2hex(text: bytes) -> bytes: ...

#: Answers whether `tag` was built to run without releasing the GIL.
__version__: str
