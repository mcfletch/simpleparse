"""The tag-table commands, as the C extension defines them.

`_module_init` copies these out of ``mxTextTools`` at import -- they are
`_const_*` there and named after ``mxte.h`` -- so nothing in the source states
them and a checker reading a tag table sees undefined names without this.

See the SimpleParse documentation for what each command matches.
"""

from typing import Dict

#: Each command by its number, for reading a table back. `0` is `Fail/Jump`,
#: which the two share.
id2cmd: Dict[int, str]

# -- Where a command starts matching from ---
Here: int
To: int
ToBOF: int
ToEOF: int
Move: int
Skip: int
Reset: int

# -- Matching characters ---
Is: int
IsNot: int
IsIn: int
IsNotIn: int
AllIn: int
AllNotIn: int
IsInSet: int
AllInSet: int
IsInCharSet: int
AllInCharSet: int

# -- Matching words ---
Word: int
WordStart: int
WordEnd: int
NoWord: int
sWordStart: int
sWordEnd: int
sFindWord: int

# -- Running another table ---
Table: int
TableInList: int
SubTable: int
SubTableInList: int
ThisTable: int

# -- Calling back into Python ---
Call: int
CallArg: int
CallTag: int

# -- Looping ---
Loop: int
LoopControl: int

# -- Flags a command carries, added to it ---
AppendToTagobj: int
AppendTagobj: int
AppendMatch: int
LookAhead: int

# -- Ending the table ---
Fail: int
Jump: int
JumpTarget: int
EOF: int
Break: int
MatchOk: int
MatchFail: int
