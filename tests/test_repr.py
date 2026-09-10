"""A compiled object can be printed.

`CharSet` and `TextSearch` are built by the C extension, and their `repr` read
the result of `PyObject_Repr` as bytes -- which it was in Python 2 and is not
now. So `repr()` of either raised `TypeError`, and with it every traceback,
every log line and every debugger view that happened to name one: the sets in
`TextTools.Constants.Sets` are module-level, so a `pdb` listing of that module
was enough.
"""

import unittest

from simpleparse.stt.TextTools.mxTextTools import CharSet, TextSearch


class TestACharacterSetPrints(unittest.TestCase):
    def test_one_built_from_text(self):
        shown = repr(CharSet('a-z'))
        self.assertIn('Character Set', shown)
        self.assertIn('a-z', shown)

    def test_one_built_from_bytes(self):
        shown = repr(CharSet(b'a-z'))
        self.assertIn('Character Set', shown)
        self.assertIn('a-z', shown)

    def test_str_answers_the_same(self):
        made = CharSet('0-9')
        self.assertEqual(str(made), repr(made))

    def test_a_shipped_set_prints(self):
        """The sets in `Constants.Sets` are built at import, so anything that
        lists that module's contents meets one."""
        from simpleparse.stt.TextTools.Constants import Sets

        self.assertIn('Character Set', repr(Sets.whitespace_charset))


class TestATextSearchPrints(unittest.TestCase):
    def test_it_names_what_it_searches_for(self):
        shown = repr(TextSearch(b'needle'))
        self.assertIn('TextSearch', shown)
        self.assertIn('needle', shown)

    def test_str_answers_the_same(self):
        made = TextSearch(b'needle')
        self.assertEqual(str(made), repr(made))


if __name__ == '__main__':
    unittest.main()
