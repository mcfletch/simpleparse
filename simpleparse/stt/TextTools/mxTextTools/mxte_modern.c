/* 
  mxte_modern.c -- Modern table driven tagging engine for Python

  This version supports the modern Python string representation with
  three separate code paths for 1-byte, 2-byte, and 4-byte characters.

  Copyright (c) 2024
*/

/* Debugging switches */
/*#define MAL_DEBUG*/
/*#define MAL_REF_DEBUG*/

/* Logging file used by debugging facility */
#ifndef MAL_DEBUG_OUTPUTFILE
# define MAL_DEBUG_OUTPUTFILE "mxTagEngine.log"
#endif

#include "mx.h"
#include "mxstdlib.h"
#include "mxTextTools.h"
#include "mxte_modern.h"
#include "mxbm_modern.h"

/* Forward declarations for search API functions */
Py_ssize_t mxTextSearch_SearchBuffer_1BYTE(PyObject *self,
                                           TE_CHAR_1BYTE *text,
                                           Py_ssize_t start,
                                           Py_ssize_t stop,
                                           Py_ssize_t *sliceleft,
                                           Py_ssize_t *sliceright);

Py_ssize_t mxTextSearch_SearchUnicode_2BYTE(PyObject *self,
                                                   TE_CHAR_2BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright);

Py_ssize_t mxTextSearch_SearchUnicode_4BYTE(PyObject *self,
                                                   TE_CHAR_4BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright);

/* --- Tagging Engine --- 1-byte version (bytes and 1-byte unicode) ------- */

#undef TE_STRING_CHECK 
#define TE_STRING_CHECK(obj) (PyBytes_Check(obj) || (PyUnicode_Check(obj) && mxte_get_string_kind(obj) == TE_KIND_1BYTE))
#undef TE_STRING_AS_STRING
#define TE_STRING_AS_STRING(obj) ((TE_CHAR*)mxte_get_string_data(obj, TE_KIND_1BYTE))
#undef TE_STRING_GET_SIZE
#define TE_STRING_GET_SIZE(obj) mxte_get_string_length(obj, TE_KIND_1BYTE)
#undef TE_STRING_FROM_STRING
#define TE_STRING_FROM_STRING(str, size) mxte_create_string(str, size, TE_KIND_1BYTE)
#undef TE_CHAR
#define TE_CHAR TE_CHAR_1BYTE
#undef TE_HANDLE_MATCH
#define TE_HANDLE_MATCH string_handle_match_1byte
#undef TE_ENGINE_API
#define TE_ENGINE_API mxTextTools_TaggingEngine_1BYTE
#undef TE_TABLETYPE
#define TE_TABLETYPE MXTAGTABLE_STRINGTYPE
#undef TE_SEARCHAPI
#define TE_SEARCHAPI mxTextSearch_SearchBuffer_1BYTE
#undef TE_KIND
#define TE_KIND TE_KIND_1BYTE

#include "mxte_impl.h"

/* --- Tagging Engine --- 2-byte Unicode version -------------------------- */

#undef TE_STRING_CHECK 
#define TE_STRING_CHECK(obj) (PyUnicode_Check(obj) && mxte_get_string_kind(obj) == TE_KIND_2BYTE)
#undef TE_STRING_AS_STRING
#define TE_STRING_AS_STRING(obj) ((TE_CHAR*)mxte_get_string_data(obj, TE_KIND_2BYTE))
#undef TE_STRING_GET_SIZE
#define TE_STRING_GET_SIZE(obj) mxte_get_string_length(obj, TE_KIND_2BYTE)
#undef TE_STRING_FROM_STRING
#define TE_STRING_FROM_STRING(str, size) mxte_create_string(str, size, TE_KIND_2BYTE)
#undef TE_CHAR
#define TE_CHAR TE_CHAR_2BYTE
#undef TE_HANDLE_MATCH
#define TE_HANDLE_MATCH unicode_handle_match_2byte
#undef TE_ENGINE_API
#define TE_ENGINE_API mxTextTools_TaggingEngine_2BYTE
#undef TE_TABLETYPE
#define TE_TABLETYPE MXTAGTABLE_UNICODETYPE
#undef TE_SEARCHAPI
#define TE_SEARCHAPI mxTextSearch_SearchUnicode_2BYTE
#undef TE_KIND
#define TE_KIND TE_KIND_2BYTE

#include "mxte_impl.h"

/* --- Tagging Engine --- 4-byte Unicode version -------------------------- */

#undef TE_STRING_CHECK 
#define TE_STRING_CHECK(obj) (PyUnicode_Check(obj) && mxte_get_string_kind(obj) == TE_KIND_4BYTE)
#undef TE_STRING_AS_STRING
#define TE_STRING_AS_STRING(obj) ((TE_CHAR*)mxte_get_string_data(obj, TE_KIND_4BYTE))
#undef TE_STRING_GET_SIZE
#define TE_STRING_GET_SIZE(obj) mxte_get_string_length(obj, TE_KIND_4BYTE)
#undef TE_STRING_FROM_STRING
#define TE_STRING_FROM_STRING(str, size) mxte_create_string(str, size, TE_KIND_4BYTE)
#undef TE_CHAR
#define TE_CHAR TE_CHAR_4BYTE
#undef TE_HANDLE_MATCH
#define TE_HANDLE_MATCH unicode_handle_match_4byte
#undef TE_ENGINE_API
#define TE_ENGINE_API mxTextTools_TaggingEngine_4BYTE
#undef TE_TABLETYPE
#define TE_TABLETYPE MXTAGTABLE_UNICODETYPE
#undef TE_SEARCHAPI
#define TE_SEARCHAPI mxTextSearch_SearchUnicode_4BYTE
#undef TE_KIND
#define TE_KIND TE_KIND_4BYTE

#include "mxte_impl.h"

/* --- Dispatcher function ------------------------------------------------ */

/* This function determines the string kind and calls the appropriate engine */
int mxTextTools_TaggingEngine_Modern(PyObject *text,
                                     Py_ssize_t start,
                                     Py_ssize_t text_len,
                                     mxTagTableObject *tagtable,
                                     PyObject *taglist,
                                     PyObject *context,
                                     Py_ssize_t *next)
{
    int kind;
    
    /* Determine string kind */
    kind = mxte_get_string_kind(text);
    
    switch (kind) {
        case TE_KIND_1BYTE:
            return mxTextTools_TaggingEngine_1BYTE(text, start, text_len, 
                                                   tagtable, taglist, context, next);
        case TE_KIND_2BYTE:
            return mxTextTools_TaggingEngine_2BYTE(text, start, text_len,
                                                   tagtable, taglist, context, next);
        case TE_KIND_4BYTE:
            return mxTextTools_TaggingEngine_4BYTE(text, start, text_len,
                                                   tagtable, taglist, context, next);
        default:
            PyErr_SetString(PyExc_TypeError, 
                           "text must be a bytes or unicode string");
            return 0; /* Error return value */
    }
}

/* Unicode-specific dispatcher for backward compatibility */
int mxTextTools_UnicodeTaggingEngine_Modern(PyObject *text,
                                            Py_ssize_t start,
                                            Py_ssize_t text_len,
                                            mxTagTableObject *tagtable,
                                            PyObject *taglist,
                                            PyObject *context,
                                            Py_ssize_t *next)
{
    return mxTextTools_TaggingEngine_Modern(text, start, text_len,
                                           tagtable, taglist, context, next);
}

/* Legacy API compatibility - redirect to modern implementation */
int mxTextTools_UnicodeTaggingEngine(PyObject *text,
                                     Py_ssize_t start,
                                     Py_ssize_t text_len,
                                     mxTagTableObject *tagtable,
                                     PyObject *taglist,
                                     PyObject *context,
                                     Py_ssize_t *next)
{
    return mxTextTools_TaggingEngine_Modern(text, start, text_len,
                                           tagtable, taglist, context, next);
}

/* Non-Unicode tagging engine for backward compatibility */
int mxTextTools_TaggingEngine(PyObject *text,
                              Py_ssize_t start,
                              Py_ssize_t text_len,
                              mxTagTableObject *tagtable,
                              PyObject *taglist,
                              PyObject *context,
                              Py_ssize_t *next)
{
    return mxTextTools_TaggingEngine_Modern(text, start, text_len,
                                           tagtable, taglist, context, next);
}

/* --- Search API wrappers ------------------------------------------------ */

/* 1-byte search wrapper */
Py_ssize_t mxTextSearch_SearchBuffer_1BYTE(PyObject *self,
                                           TE_CHAR_1BYTE *text,
                                           Py_ssize_t start,
                                           Py_ssize_t stop,
                                           Py_ssize_t *sliceleft,
                                           Py_ssize_t *sliceright)
{
    mxTextSearchObject *so = (mxTextSearchObject *)self;
    
    /* If the TextSearch was created with a Unicode pattern, we need to handle this specially */
    if (PyUnicode_Check(so->match)) {
        /* For Unicode patterns, use Unicode-aware search */
        Py_ssize_t match_len = PyUnicode_GET_LENGTH(so->match);
        if (match_len == 0) {
            /* Empty pattern matches at start position */
            *sliceleft = start;
            *sliceright = start;
            return 1;
        }
        
        if (PyUnicode_READY(so->match) < 0) {
            return -1;
        }
        
        /* Handle edge cases */
        if (start > stop || start < 0) {
            return 0; /* No match possible */
        }
        
        /* Boyer-Moore available but disabled by default - benchmarks showed no performance benefit
         * and slight overhead for typical SimpleParse use cases. Can still be explicitly enabled. */
        if (0 && so->algorithm == MXTEXTSEARCH_BOYERMOORE_MODERN && match_len >= 2) {
            /* Use modern Boyer-Moore search for 1-byte Unicode */
            mxbm_context_unicode *bm_ctx = (mxbm_context_unicode *)so->data;
            
            if (bm_ctx && bm_ctx->kind == MXBM_KIND_1BYTE) {
                Py_ssize_t result = mxbm_search_1byte(&bm_ctx->u.ctx_1byte, text, stop, start);
                
                if (result >= 0) {
                    *sliceleft = result;
                    *sliceright = result + match_len;
                    return 1;
                }
                return 0; /* No match found */
            }
        }
        
        /* Fall back to simple brute force search for 1-byte characters with Unicode pattern */
        for (Py_ssize_t i = start; i <= stop - match_len; i++) {
            Py_ssize_t j;
            for (j = 0; j < match_len; j++) {
                Py_UCS4 text_char = (Py_UCS4)text[i + j];
                Py_UCS4 match_char = PyUnicode_READ(PyUnicode_KIND(so->match), PyUnicode_DATA(so->match), j);
                if (text_char != match_char) {
                    break;
                }
            }
            if (j == match_len) {
                *sliceleft = i;
                *sliceright = i + match_len;
                return 1;
            }
        }
        
        return 0; /* No match found */
    }
    
    /* Call the original implementation for byte-based searches */
    return mxTextSearch_SearchBuffer(self, (char*)text, start, stop, sliceleft, sliceright);
}

/* 2-byte search wrapper */
Py_ssize_t mxTextSearch_SearchUnicode_2BYTE(PyObject *self,
                                                   TE_CHAR_2BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright)
{
    /* For now, we'll implement a basic 2-byte search by creating a temporary 2-byte string
       and using the Boyer-Moore or trivial search approach adapted for 2-byte chars */
    
    /* This is a simplified implementation - for production, we'd want optimized 
       2-byte search algorithms */
    
    mxTextSearchObject *so = (mxTextSearchObject *)self;
    
    /* Get the match pattern from the search object */
    if (!PyUnicode_Check(so->match)) {
        return -1; /* Error: can't search for non-Unicode in Unicode text */
    }
    
    /* For now, fall back to trivial search implementation */
    /* TODO: Implement optimized 2-byte search algorithms */
    
    Py_ssize_t match_len = PyUnicode_GET_LENGTH(so->match);
    if (match_len == 0) {
        /* Empty pattern matches at start position */
        *sliceleft = start;
        *sliceright = start;
        return 1;
    }
    
    if (PyUnicode_READY(so->match) < 0) {
        return -1;
    }
    
    /* Handle edge cases */
    if (start > stop || start < 0) {
        return 0; /* No match possible */
    }
    
    /* Simple brute force search for 2-byte characters */
    for (Py_ssize_t i = start; i <= stop - match_len; i++) {
        Py_ssize_t j;
        for (j = 0; j < match_len; j++) {
            Py_UCS4 text_char = text[i + j];
            Py_UCS4 match_char = PyUnicode_READ(PyUnicode_KIND(so->match), PyUnicode_DATA(so->match), j);
            if (text_char != match_char) {
                break;
            }
        }
        if (j == match_len) {
            *sliceleft = i;
            *sliceright = i + match_len;
            return 1;
        }
    }
    
    return 0; /* No match found */
}

/* 4-byte search wrapper */
Py_ssize_t mxTextSearch_SearchUnicode_4BYTE(PyObject *self,
                                                   TE_CHAR_4BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright)
{
    /* Native 4-byte search implementation */
    mxTextSearchObject *so = (mxTextSearchObject *)self;
    
    /* Get the match pattern from the search object */
    if (!PyUnicode_Check(so->match)) {
        return -1; /* Error: can't search for non-Unicode in Unicode text */
    }
    
    /* For now, fall back to trivial search implementation */
    /* TODO: Implement optimized 4-byte search algorithms */
    
    Py_ssize_t match_len = PyUnicode_GET_LENGTH(so->match);
    if (match_len == 0) {
        /* Empty pattern matches at start position */
        *sliceleft = start;
        *sliceright = start;
        return 1;
    }
    
    if (PyUnicode_READY(so->match) < 0) {
        return -1;
    }
    
    /* Handle edge cases */
    if (start > stop || start < 0) {
        return 0; /* No match possible */
    }
    
    /* Simple brute force search for 4-byte characters */
    for (Py_ssize_t i = start; i <= stop - match_len; i++) {
        Py_ssize_t j;
        for (j = 0; j < match_len; j++) {
            Py_UCS4 text_char = text[i + j];
            Py_UCS4 match_char = PyUnicode_READ(PyUnicode_KIND(so->match), PyUnicode_DATA(so->match), j);
            if (text_char != match_char) {
                break;
            }
        }
        if (j == match_len) {
            *sliceleft = i;
            *sliceright = i + match_len;
            return 1;
        }
    }
    
    return 0; /* No match found */
}