"""Shared escape sequence utilities for SimpleParse

This module provides common escape sequence mappings used by both
the grammar parser and string literal processing.
"""

# Map of escape sequences to their corresponding characters
# Used for interpreting backslash escapes in string literals and grammar definitions
SPECIAL_ESCAPED_MAP = {
    'a': '\a',   # Bell
    'b': '\b',   # Backspace
    'f': '\f',   # Form feed
    'n': '\n',   # Newline
    'r': '\r',   # Carriage return
    't': '\t',   # Tab
    'v': '\v',   # Vertical tab
    '\\': '\\',  # Backslash
    '\n': '',    # Escaped newline (line continuation)
    '"': '"',    # Double quote
    "'": "'",    # Single quote
}
