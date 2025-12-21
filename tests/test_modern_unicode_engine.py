#!/usr/bin/env python3
"""
Modern Unicode engine tests for SimpleParse

Tests the modern tri-engine system (1-byte, 2-byte, 4-byte) and validates
that different string types are handled correctly.
"""

import pytest
from simpleparse import parser
from simpleparse.stt.TextTools import TextSearch


class TestModernUnicodeEngine:
    """Test modern Unicode engine functionality"""
    
    def test_string_type_parsing(self):
        """Test that different Unicode string types are parsed correctly"""
        grammar = """
        word := [a-zA-Z0-9]+
        """

        p = parser.Parser(grammar, 'word')

        # Test cases that should match [a-zA-Z0-9]+
        test_cases = [
            ("hello", "ASCII"),
            ("test123", "Mixed ASCII"),
        ]

        for text, description in test_cases:
            result = p.parse(text)

            # Result is (success, taglist, end_position)
            assert result is not None, f"Failed to parse {description}: {text!r}"
            assert len(result) == 3, f"Unexpected result format for {description}: {text!r}"

            success, taglist, end_pos = result
            assert success == 1, f"Parse failed for {description}: {text!r}"
            assert isinstance(end_pos, int), f"Invalid end position type for {text!r}"
            assert end_pos == len(text), f"Not all content consumed for {description}: {text!r}"

        # Test cases with non-ASCII characters (these won't fully match [a-zA-Z0-9]+)
        # but should still be handled without crashing
        unicode_cases = [
            ("café", "Latin-1"),
            ("你好", "BMP Unicode"),
            ("🎉", "Astral Unicode"),
            ("résumé", "Mixed Latin-1"),
        ]

        for text, description in unicode_cases:
            # These may or may not match fully, but should not crash
            try:
                result = p.parse(text)
                assert result is not None, f"None result for {description}: {text!r}"
            except Exception as e:
                pytest.fail(f"Exception parsing {description} {text!r}: {e}")
    
    def test_bytes_vs_unicode_parsing(self):
        """Test that bytes and unicode strings both work"""
        grammar = """
        number := [0-9]+
        """
        
        p = parser.Parser(grammar, 'number')
        
        test_pairs = [
            (b"123", "123"),
            (b"456", "456"), 
            (b"789", "789"),
        ]
        
        for byte_text, unicode_text in test_pairs:
            # Test Unicode version (bytes need to be decoded for SimpleParse)
            unicode_result = p.parse(unicode_text)
            assert unicode_result is not None, f"Unicode parsing failed for {unicode_text!r}"
            
            # Test bytes by decoding first
            decoded_text = byte_text.decode('utf-8')
            bytes_result = p.parse(decoded_text)
            assert bytes_result is not None, f"Bytes parsing failed for {byte_text!r}"
            
            # Results should be equivalent
            assert unicode_result == bytes_result, f"Bytes and Unicode results differ for {unicode_text!r}"
    
    def test_latin1_optimization(self):
        """Test Latin-1 character optimization in TextSearch"""
        latin1_chars = [
            'é',    # U+00E9 - Latin Small Letter E with Acute
            'ñ',    # U+00F1 - Latin Small Letter N with Tilde  
            'ü',    # U+00FC - Latin Small Letter U with Diaeresis
            '§',    # U+00A7 - Section Sign
            'ç',    # U+00E7 - Latin Small Letter C with Cedilla
        ]
        
        for char in latin1_chars:
            # Verify this is a Latin-1 character
            char_code = ord(char)
            assert 128 <= char_code <= 255, f"Character {char!r} is not in Latin-1 extended range"
            
            # Test search functionality
            text = f"Hello {char} world!"
            search_obj = TextSearch(char)
            result = search_obj.search(text, 0, len(text))
            
            # Should find the character
            assert result is not None, f"Failed to find Latin-1 character {char!r}"
            assert len(result) == 2, f"Invalid result format for {char!r}"
            
            start, end = result
            expected_pos = text.find(char)
            assert start == expected_pos, f"Wrong position for {char!r}: expected {expected_pos}, got {start}"
            assert end == expected_pos + 1, f"Wrong end position for {char!r}"
    
    def test_unicode_character_ranges(self):
        """Test different Unicode character ranges"""
        test_cases = [
            # (char, expected_byte_width, description)
            ('A', 1, "ASCII"),
            ('é', 1, "Latin-1"), 
            ('你', 2, "BMP Unicode"),
            ('🎉', 4, "Astral plane"),
        ]
        
        for char, expected_width, description in test_cases:
            # Test with TextSearch
            text = f"Find {char} here"
            search_obj = TextSearch(char)
            result = search_obj.search(text, 0, len(text))
            
            assert result is not None, f"Failed to find {description} character {char!r}"
            start, end = result
            assert text[start:end] == char, f"Wrong substring extracted for {char!r}"
            
            # Test with parsing
            grammar = f"""
            target := '{char}'
            """
            try:
                p = parser.Parser(grammar, 'target')
                parse_result = p.parse(char)
                assert parse_result is not None, f"Failed to parse {description} character {char!r}"
            except Exception as e:
                pytest.fail(f"Parser failed for {description} character {char!r}: {e}")
    
    def test_mixed_unicode_content(self):
        """Test parsing with mixed Unicode content"""
        grammar = """
        document := word+
        word := [a-zA-Z0-9\u00C0-\u017F\u4E00-\u9FFF\U0001F300-\U0001F6FF]+
        """
        
        p = parser.Parser(grammar, 'document')
        
        test_documents = [
            "hello world",
            "café résumé", 
            "你好世界",
            "hello café 🎉",
            "mixed 中文 content 🌟",
        ]
        
        for doc in test_documents:
            try:
                result = p.parse(doc)
                assert result is not None, f"Failed to parse mixed content: {doc!r}"
            except Exception as e:
                pytest.fail(f"Exception parsing mixed content {doc!r}: {e}")
    
    def test_empty_and_edge_cases(self):
        """Test empty strings and edge cases"""
        grammar = """
        optional := [a-z]*
        """
        
        p = parser.Parser(grammar, 'optional')
        
        # Test empty string
        result = p.parse("")
        assert result is not None, "Failed to parse empty string"
        
        # Test single character
        result = p.parse("a")
        assert result is not None, "Failed to parse single character"
        
        # Test TextSearch with empty pattern (should handle gracefully)
        try:
            search_obj = TextSearch("")
            result = search_obj.search("test", 0, 4)
            # Empty pattern should match at position 0
            assert result == (0, 0), f"Unexpected result for empty pattern: {result}"
        except Exception as e:
            pytest.fail(f"Empty pattern search failed: {e}")
    
    def test_large_unicode_strings(self):
        """Test with larger Unicode strings to verify engine scalability"""
        # Create test strings of various sizes and types - use only alphanumeric
        # characters that the grammar can match
        test_cases = [
            ("a" * 1000, "ASCII repeated"),
            ("abc123" * 166, "Mixed alphanumeric repeated"),
        ]

        grammar = """
        content := [a-zA-Z0-9]+
        """

        p = parser.Parser(grammar, 'content')

        for text, description in test_cases:
            result = p.parse(text)
            assert result is not None, f"Failed to parse {description} (length: {len(text)})"

            # Result is (success, taglist, end_position)
            if result:
                success, taglist, end_pos = result
                assert success == 1, f"Parse failed for {description}"
                assert end_pos == len(text), f"Not all content consumed for {description}"


class TestTextSearchModern:
    """Test TextSearch with modern Unicode engine"""
    
    def test_search_accuracy(self):
        """Test search accuracy across different character types"""
        test_cases = [
            ("hello world", "world", 6),
            ("café résumé", "résumé", 5),
            ("你好世界", "世界", 2), 
            ("test 🎉 end", "🎉", 5),
        ]
        
        for text, pattern, expected_start in test_cases:
            search_obj = TextSearch(pattern)
            result = search_obj.search(text, 0, len(text))
            
            assert result is not None, f"Pattern {pattern!r} not found in {text!r}"
            start, end = result
            assert start == expected_start, f"Wrong start position: expected {expected_start}, got {start}"
            assert text[start:end] == pattern, f"Wrong substring: expected {pattern!r}, got {text[start:end]!r}"
    
    def test_no_match_cases(self):
        """Test cases where pattern should not be found"""
        test_cases = [
            ("hello", "xyz"),
            ("café", "tea"),
            ("你好", "世界"),
        ]

        for text, pattern in test_cases:
            search_obj = TextSearch(pattern)
            result = search_obj.search(text, 0, len(text))
            assert result == (0, 0), f"False positive: found {pattern!r} in {text!r}"


class TestEncodingParameter:
    """Test the encoding parameter for parsing bytes with Unicode grammars"""

    def test_utf8_ascii_literal(self):
        """Parse UTF-8 bytes with ASCII pattern"""
        grammar = """target := 'hello'"""
        p = parser.Parser(grammar, 'target')

        result = p.parse(b'hello world', encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == 5, "Expected 5 bytes consumed"

    def test_utf8_bmp_literal(self):
        """Parse UTF-8 bytes with BMP Unicode literal"""
        grammar = """target := '你'"""
        p = parser.Parser(grammar, 'target')

        text = '你好'.encode('utf-8')  # b'\xe4\xbd\xa0\xe5\xa5\xbd'
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == 3, "Expected 3 bytes (UTF-8 encoding of 你)"

    def test_utf8_astral_literal(self):
        """Parse UTF-8 bytes with astral plane character"""
        grammar = """target := '🎉'"""
        p = parser.Parser(grammar, 'target')

        text = '🎉test'.encode('utf-8')  # b'\xf0\x9f\x8e\x89test'
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == 4, "Expected 4 bytes (UTF-8 encoding of 🎉)"

    def test_utf8_multi_char_literal(self):
        """Parse UTF-8 bytes with multi-character pattern"""
        grammar = """target := '你好世界'"""
        p = parser.Parser(grammar, 'target')

        text = '你好世界'.encode('utf-8')
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == 12, "Expected 12 bytes (4 chars * 3 bytes each)"

    def test_latin1_encoding(self):
        """Parse Latin-1 bytes with Unicode pattern"""
        grammar = """target := 'café'"""
        p = parser.Parser(grammar, 'target')

        text = 'café'.encode('latin-1')  # b'caf\xe9'
        result = p.parse(text, encoding='latin-1')
        assert result[0] == 1, "Expected match"
        assert result[2] == 4, "Expected 4 bytes"

    def test_latin1_extended_chars(self):
        """Parse Latin-1 bytes with extended characters"""
        grammar = """target := 'résumé'"""
        p = parser.Parser(grammar, 'target')

        text = 'résumé'.encode('latin-1')
        result = p.parse(text, encoding='latin-1')
        assert result[0] == 1, "Expected match"
        assert result[2] == 6, "Expected 6 bytes"

    def test_ascii_range_utf8(self):
        """ASCII character range works with UTF-8 encoding"""
        grammar = """word := [a-zA-Z]+"""
        p = parser.Parser(grammar, 'word')

        result = p.parse(b'HelloWorld', encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == 10, "Expected 10 bytes"

    def test_ascii_range_stops_at_multibyte(self):
        """ASCII range stops at UTF-8 multi-byte characters"""
        grammar = """word := [a-zA-Z]+"""
        p = parser.Parser(grammar, 'word')

        text = 'Hello世界'.encode('utf-8')
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == 5, "Expected 5 bytes (stops at Chinese chars)"

    def test_latin1_extended_range(self):
        """Latin-1 extended character range"""
        grammar = """word := [a-zàáâãäåèéêë]+"""
        p = parser.Parser(grammar, 'word')

        text = 'caféètude'.encode('latin-1')
        result = p.parse(text, encoding='latin-1')
        assert result[0] == 1, "Expected match"
        assert result[2] == 9, "Expected 9 bytes"

    def test_byte_positions_not_char_positions(self):
        """Verify positions are byte offsets, not character offsets"""
        grammar = """target := 'b'"""
        p = parser.Parser(grammar, 'target')

        # '你b' = b'\xe4\xbd\xa0b' - 'b' is at byte position 3
        text = '你b'.encode('utf-8')
        result = p.parse(text, start=3, encoding='utf-8')
        assert result[0] == 1, "Expected match at byte position 3"
        assert result[2] == 4, "Expected to end at byte position 4"

    def test_encoding_requires_bytes(self):
        """Encoding parameter requires bytes input"""
        grammar = """target := 'hello'"""
        p = parser.Parser(grammar, 'target')

        with pytest.raises(TypeError):
            p.parse('hello', encoding='utf-8')  # String, not bytes

    def test_disallowed_encodings(self):
        """Multi-byte encodings other than UTF-8 are rejected"""
        grammar = """target := 'hello'"""
        p = parser.Parser(grammar, 'target')

        disallowed = ['utf-16', 'shift-jis', 'gb2312', 'big5', 'euc-jp']
        for enc in disallowed:
            with pytest.raises(ValueError):
                p.parse(b'hello', encoding=enc)

    def test_unknown_encoding(self):
        """Unknown encoding raises LookupError"""
        grammar = """target := 'hello'"""
        p = parser.Parser(grammar, 'target')

        with pytest.raises(LookupError):
            p.parse(b'hello', encoding='nonexistent-encoding')

    def test_windows_1252_encoding(self):
        """Windows-1252 encoding works"""
        grammar = """target := '"test"'"""  # Smart quotes
        p = parser.Parser(grammar, 'target')

        # Windows-1252 smart quotes: " is 0x93, " is 0x94
        text = '"test"'.encode('windows-1252')
        result = p.parse(text, encoding='windows-1252')
        assert result[0] == 1, "Expected match"

    def test_iso8859_15_euro(self):
        """ISO-8859-15 encoding with Euro sign"""
        grammar = """target := '€'"""
        p = parser.Parser(grammar, 'target')

        # In ISO-8859-15, € is byte 0xA4
        text = '€'.encode('iso-8859-15')
        result = p.parse(text, encoding='iso-8859-15')
        assert result[0] == 1, "Expected match"
        assert result[2] == 1, "Expected 1 byte"

    def test_mixed_grammar_utf8(self):
        """Complex grammar with UTF-8 encoding"""
        grammar = """
        document := (word / space / punct)+
        word := [a-zA-Z]+
        space := [ \\t\\n]+
        punct := [.,!?]+
        """
        p = parser.Parser(grammar, 'document')

        text = b'Hello, World!'
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == len(text), "Expected full consumption"

    def test_subtable_encoding(self):
        """Nested tables work with encoding"""
        grammar = """
        sentence := word, (space, word)*
        word := [a-z]+
        space := ' '
        """
        p = parser.Parser(grammar, 'sentence')

        text = b'hello world test'
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Expected match"
        assert result[2] == len(text), "Expected full consumption"


class TestUTF8CharRangeEdgeCases:
    """Edge case tests for UTF-8 character range matching with encoding parameter.

    These tests verify correct behavior at range boundaries where UTF-8 encoding
    could cause incorrect matches if not properly decoded.
    """

    def test_cjk_range_lower_boundary(self):
        """Character at lower boundary of range should match"""
        # 你 is U+4F60, the lower boundary
        grammar = """target := [你-龟]+"""
        p = parser.Parser(grammar, 'target')

        text = '你'.encode('utf-8')  # b'\xe4\xbd\xa0'
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Lower boundary character should match"
        assert result[2] == 3, "Expected 3 bytes"

    def test_cjk_range_upper_boundary(self):
        """Character at upper boundary of range should match"""
        # 龟 is U+9F9F, the upper boundary
        grammar = """target := [你-龟]+"""
        p = parser.Parser(grammar, 'target')

        text = '龟'.encode('utf-8')  # b'\xe9\xbe\x9f'
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Upper boundary character should match"
        assert result[2] == 3, "Expected 3 bytes"

    def test_cjk_range_just_after_upper_boundary(self):
        """Character just after upper boundary should NOT match"""
        # 龠 is U+9FA0, just after 龟 (U+9F9F)
        grammar = """target := [你-龟]+"""
        p = parser.Parser(grammar, 'target')

        text = '龠'.encode('utf-8')  # b'\xe9\xbe\xa0' - one codepoint higher
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 0, "Character after upper boundary should NOT match"

    def test_cjk_range_just_before_lower_boundary(self):
        """Character just before lower boundary should NOT match"""
        # 仿 is U+4F3F, just before 你 (U+4F60)
        grammar = """target := [你-龟]+"""
        p = parser.Parser(grammar, 'target')

        text = '仿'.encode('utf-8')  # U+4EFF - before 你
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 0, "Character before lower boundary should NOT match"

    def test_cjk_range_middle_characters(self):
        """Characters in middle of range should match"""
        grammar = """target := [你-龟]+"""
        p = parser.Parser(grammar, 'target')

        # Test several characters in the middle of the range
        # Note: 世 (U+4E16) and 中 (U+4E2D) are BELOW 你 (U+4F60), not in range!
        # Using characters actually within U+4F60 - U+9F9F
        middle_chars = ['好', '界', '文', '國', '語']  # Various CJK chars in range
        for char in middle_chars:
            # Verify these are actually in range
            assert 0x4F60 <= ord(char) <= 0x9F9F, f"{char!r} U+{ord(char):04X} not in range"
            text = char.encode('utf-8')
            result = p.parse(text, encoding='utf-8')
            assert result[0] == 1, f"Middle character {char!r} (U+{ord(char):04X}) should match"
            assert result[2] == 3, f"Expected 3 bytes for {char!r}"

    def test_cjk_range_stops_at_boundary(self):
        """AllInCharSet should stop exactly at out-of-range character"""
        grammar = """target := [你-龟]+"""
        p = parser.Parser(grammar, 'target')

        # Use only in-range chars followed by 龠 (out of range)
        # 你好界文 are all in range (U+4F60-U+9F9F), 龠 (U+9FA0) is after range
        text = '你好界文龠'.encode('utf-8')  # 4 in-range chars + 1 out-of-range
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Should match the in-range portion"
        assert result[2] == 12, "Expected 12 bytes (4 chars * 3 bytes each)"

    def test_astral_plane_boundary(self):
        """Test 4-byte UTF-8 character range boundary"""
        # Emoji range test
        grammar = """target := [🎉-🎕]+"""  # U+1F389 to U+1F395
        p = parser.Parser(grammar, 'target')

        # 🎉 (U+1F389) should match
        text = '🎉'.encode('utf-8')
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Lower boundary emoji should match"
        assert result[2] == 4, "Expected 4 bytes"

    def test_astral_plane_out_of_range(self):
        """Astral character outside range should NOT match"""
        grammar = """target := [🎉-🎕]+"""  # U+1F389 to U+1F395
        p = parser.Parser(grammar, 'target')

        # 🎗 (U+1F397) is after 🎕 (U+1F395)
        text = '🎗'.encode('utf-8')
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 0, "Emoji after range should NOT match"

    def test_2byte_utf8_range(self):
        """Test 2-byte UTF-8 character range (Latin Extended)"""
        grammar = """target := [À-ÿ]+"""  # U+00C0 to U+00FF (Latin-1 Supplement)
        p = parser.Parser(grammar, 'target')

        # é (U+00E9) is in range
        text = 'é'.encode('utf-8')  # b'\xc3\xa9'
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Latin Extended character in range should match"
        assert result[2] == 2, "Expected 2 bytes"

    def test_2byte_utf8_out_of_range(self):
        """Character after 2-byte range should NOT match"""
        grammar = """target := [À-ÿ]+"""  # U+00C0 to U+00FF
        p = parser.Parser(grammar, 'target')

        # Ā (U+0100) is just after ÿ (U+00FF)
        text = 'Ā'.encode('utf-8')  # U+0100
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 0, "Character after range should NOT match"

    def test_mixed_byte_lengths_in_match(self):
        """Mix of 1, 2, 3, and 4 byte UTF-8 sequences"""
        # Create a range that includes characters with different UTF-8 lengths
        # Note: This tests that we properly advance by UTF-8 sequence length
        grammar = """target := [a-zà-ÿ]+"""
        p = parser.Parser(grammar, 'target')

        text = 'cafénaïve'.encode('utf-8')
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Mixed 1-2 byte sequences should match"
        # 'cafénaïve' = 'c'(1) + 'a'(1) + 'f'(1) + 'é'(2) + 'n'(1) + 'a'(1) + 'ï'(2) + 'v'(1) + 'e'(1)
        # = 1+1+1+2+1+1+2+1+1 = 11 bytes
        assert result[2] == 11, "Expected 11 bytes for cafénaïve"

    def test_invalid_utf8_stops_match(self):
        """Invalid UTF-8 sequence should stop matching"""
        grammar = """target := [a-z]+"""
        p = parser.Parser(grammar, 'target')

        # 'abc' followed by invalid UTF-8 continuation byte
        text = b'abc\x80xyz'  # \x80 is invalid start byte
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Should match valid portion"
        assert result[2] == 3, "Should stop at invalid UTF-8"

    def test_isincharset_boundary(self):
        """IsInCharSet with boundary character"""
        # Test single character match at boundary
        grammar = """target := [你-龟]"""  # Without +, matches exactly one
        p = parser.Parser(grammar, 'target')

        # 龟 (upper boundary) should match
        text = '龟test'.encode('utf-8')
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 1, "Boundary char should match with IsInCharSet"
        assert result[2] == 3, "Should consume exactly one 3-byte character"

    def test_isincharset_out_of_range(self):
        """IsInCharSet with out-of-range character should fail"""
        grammar = """target := [你-龟]"""  # Without +, matches exactly one
        p = parser.Parser(grammar, 'target')

        # 龠 (just after range) should NOT match
        text = '龠test'.encode('utf-8')
        result = p.parse(text, encoding='utf-8')
        assert result[0] == 0, "Out-of-range char should NOT match with IsInCharSet"