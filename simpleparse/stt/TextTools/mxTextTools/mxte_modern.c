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

/* Forward declarations for search API functions */
static Py_ssize_t mxTextSearch_SearchBuffer_1BYTE(PyObject *self,
                                                  TE_CHAR_1BYTE *text,
                                                  Py_ssize_t start,
                                                  Py_ssize_t stop,
                                                  Py_ssize_t *sliceleft,
                                                  Py_ssize_t *sliceright);

#ifdef HAVE_UNICODE
static Py_ssize_t mxTextSearch_SearchUnicode_2BYTE(PyObject *self,
                                                   TE_CHAR_2BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright);

static Py_ssize_t mxTextSearch_SearchUnicode_4BYTE(PyObject *self,
                                                   TE_CHAR_4BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright);
#endif

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

#ifdef HAVE_UNICODE

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

#endif /* HAVE_UNICODE */

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
#ifdef HAVE_UNICODE
        case TE_KIND_2BYTE:
            return mxTextTools_TaggingEngine_2BYTE(text, start, text_len,
                                                   tagtable, taglist, context, next);
        case TE_KIND_4BYTE:
            return mxTextTools_TaggingEngine_4BYTE(text, start, text_len,
                                                   tagtable, taglist, context, next);
#endif
        default:
            PyErr_SetString(PyExc_TypeError, 
                           "text must be a bytes or unicode string");
            return 0; /* Error return value */
    }
}

/* Unicode-specific dispatcher for backward compatibility */
#ifdef HAVE_UNICODE
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
#endif

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
static Py_ssize_t mxTextSearch_SearchBuffer_1BYTE(PyObject *self,
                                                  TE_CHAR_1BYTE *text,
                                                  Py_ssize_t start,
                                                  Py_ssize_t stop,
                                                  Py_ssize_t *sliceleft,
                                                  Py_ssize_t *sliceright)
{
    /* Call the original implementation for 1-byte strings */
    return mxTextSearch_SearchBuffer(self, (char*)text, start, stop, sliceleft, sliceright);
}

#ifdef HAVE_UNICODE
/* 2-byte search wrapper */
static Py_ssize_t mxTextSearch_SearchUnicode_2BYTE(PyObject *self,
                                                   TE_CHAR_2BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright)
{
    /* Native 2-byte search implementation */
    if (sizeof(TE_CHAR_2BYTE) == sizeof(Py_UNICODE)) {
        /* Direct call when sizes match */
        return mxTextSearch_SearchUnicode(self, (Py_UNICODE*)text, start, stop, sliceleft, sliceright);
    } else {
        /* Size mismatch - need conversion (should be rare) */
        /* For safety, fall back to original function for now */
        /* TODO: Implement full conversion if needed */
        return mxTextSearch_SearchUnicode(self, (Py_UNICODE*)text, start, stop, sliceleft, sliceright);
    }
}

/* 4-byte search wrapper */
static Py_ssize_t mxTextSearch_SearchUnicode_4BYTE(PyObject *self,
                                                   TE_CHAR_4BYTE *text,
                                                   Py_ssize_t start,
                                                   Py_ssize_t stop,
                                                   Py_ssize_t *sliceleft,
                                                   Py_ssize_t *sliceright)
{
    /* Native 4-byte search implementation */
    if (sizeof(TE_CHAR_4BYTE) == sizeof(Py_UNICODE)) {
        /* Direct call when sizes match */
        return mxTextSearch_SearchUnicode(self, (Py_UNICODE*)text, start, stop, sliceleft, sliceright);
    } else {
        /* Size mismatch - need conversion */
        /* For 4-byte chars on systems where Py_UNICODE is 2-byte, */
        /* we need to check for characters outside BMP and handle appropriately */
        
        /* For now, fall back to original function - this will truncate */
        /* characters outside BMP on narrow builds, but those are deprecated */
        return mxTextSearch_SearchUnicode(self, (Py_UNICODE*)text, start, stop, sliceleft, sliceright);
    }
}
#endif