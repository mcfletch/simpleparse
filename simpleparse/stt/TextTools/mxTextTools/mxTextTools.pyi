"""What the mxTextTools C extension exports.

The tagging engine and the byte-level string tools the parser is built on.
Everything here is implemented in ``mxTextTools.c``; the declarations are here
so that a checker reading a program built on SimpleParse -- or SimpleParse's
own modules, which reach these through a star import -- can see what it holds.

Signatures follow the extension's own docstrings. The text these work on is
``bytes``: the tagging engine matches byte ranges and character sets, and a
caller with text encodes it first (``simpleparse.parser`` does).
"""

from typing import Any, Sequence

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
tagtable_cache: dict[Any, Any]

class Error(Exception):
    """Raised for a malformed tag table or a command it cannot run."""

class CharSetType:
    """A compiled character set; see :func:`CharSet`."""

    #: What it was built from, which is what pickling it stores.
    definition: str | bytes

    def contains(self, character: bytes) -> int: ...
    def search(self, text: bytes, direction: int = ..., start: int = ...,
               stop: int = ...) -> int | None: ...
    def match(self, text: bytes, direction: int = ..., start: int = ...,
              stop: int = ...) -> int: ...
    def split(self, text: bytes, start: int = ...,
              stop: int = ...) -> list[bytes]: ...
    def splitx(self, text: bytes, start: int = ...,
               stop: int = ...) -> list[bytes]: ...
    def strip(self, text: bytes, mode: int = ..., start: int = ...,
              stop: int = ...) -> bytes: ...

class TagTableType:
    """A compiled tag table; see :func:`TagTable`."""

    def compiled(self) -> tuple[Any, ...]: ...

class TextSearchType:
    """A compiled search for one string; see :func:`TextSearch`."""

    #: What it searches for, and how -- the three a pickle stores.
    match: str | bytes
    translate: bytes | None
    algorithm: int

    def search(self, text: bytes, start: int = ...,
               stop: int = ...) -> tuple[int, int]: ...
    def find(self, text: bytes, start: int = ...,
             stop: int = ...) -> int: ...
    def findall(self, text: bytes, start: int = ...,
                stop: int = ...) -> list[tuple[int, int]]: ...

#: Takes the set either way: `'a-z'` as well as `b'a-z'`.
def CharSet(definition: str | bytes) -> CharSetType: ...
def TagTable(definition: Sequence[Any],
             cachable: int = ...) -> TagTableType: ...
def UnicodeTagTable(definition: Sequence[Any],
                    cachable: int = ...) -> TagTableType: ...
#: Takes what it searches for either way: `'needle'` as well as `b'needle'`.
def TextSearch(match: str | bytes, translate: bytes | None = ...,
               algorithm: int = ...) -> TextSearchType: ...

def tag(text: bytes, tagtable: Any, sliceleft: int = ..., sliceright: int = ...,
        taglist: list[Any] | None = ..., context: Any = ...,
        encoding: Any = ...) -> tuple[int, list[Any], int]:
    """Run a tag table over text: ``(matched, taglist, position)``."""

def join(joinlist: Sequence[Any], sep: bytes = ..., start: int = ...,
         stop: int = ...) -> bytes: ...
def joinlist(text: bytes, list: Sequence[Any], start: int = ...,
             stop: int = ...) -> list[Any]: ...
def set(string: bytes, logic: int = ...) -> bytes: ...
def setfind(text: bytes, set: bytes, start: int = ...,
            stop: int = ...) -> int | None: ...
def setsplit(text: bytes, set: bytes, start: int = ...,
             stop: int = ...) -> list[bytes]: ...
def setsplitx(text: bytes, set: bytes, start: int = ...,
              stop: int = ...) -> list[bytes]: ...
def setstrip(text: bytes, set: bytes, start: int = ..., stop: int = ...,
             mode: int = ...) -> bytes: ...
def splitat(text: bytes, char: bytes, nth: int = ..., start: int = ...,
            stop: int = ...) -> tuple[bytes, bytes]: ...
def charsplit(text: bytes, char: bytes, start: int = ...,
              stop: int = ...) -> list[bytes]: ...
def prefix(text: bytes, prefixes: Sequence[bytes], start: int = ...,
           stop: int = ..., translate: bytes = ...) -> bytes | None: ...
def suffix(text: bytes, suffixes: Sequence[bytes], start: int = ...,
           stop: int = ..., translate: bytes = ...) -> bytes | None: ...
def cmp(a: Any, b: Any) -> int: ...
def hex2str(text: bytes) -> bytes: ...
def str2hex(text: bytes) -> bytes: ...

#: Answers whether `tag` was built to run without releasing the GIL.
__version__: str
