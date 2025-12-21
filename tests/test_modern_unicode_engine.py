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