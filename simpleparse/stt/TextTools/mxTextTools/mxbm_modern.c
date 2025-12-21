/*
  mxbm_modern.c -- Modern Boyer-Moore String Search Implementation
  
  A clean, efficient Boyer-Moore implementation with different optimization
  strategies for different character widths:
  
  - 1-byte: Classic dense jump table (256 entries)
  - 2-byte: Hash table for sparse character space  
  - 4-byte: Hybrid cache + fallback approach
  - Bytes: Raw byte search with dense table
  
  Copyright (c) 2024 SimpleParse Project
  License: MIT License
*/

#include "mxbm_modern.h"
#include "mxte_modern.h"
#include <string.h>
#include <stdlib.h>

/* ========================================================================= */
/* UTILITY FUNCTIONS */
/* ========================================================================= */

static int mxbm_chars_equal(const void *a, const void *b, int char_size) {
    switch (char_size) {
        case 1: return *(const mxbm_char_1byte*)a == *(const mxbm_char_1byte*)b;
        case 2: return *(const mxbm_char_2byte*)a == *(const mxbm_char_2byte*)b;
        case 4: return *(const mxbm_char_4byte*)a == *(const mxbm_char_4byte*)b;
        default: return 0;
    }
}

static void mxbm_build_border_array(const void *pattern, Py_ssize_t pattern_len, 
                                    Py_ssize_t *border, int char_size) {
    /* Build the border array for good suffix preprocessing */
    border[0] = -1;
    
    for (Py_ssize_t i = 1; i < pattern_len; i++) {
        Py_ssize_t j = border[i - 1];
        
        while (j >= 0 && !mxbm_chars_equal(
            (const char*)pattern + i * char_size,
            (const char*)pattern + (j + 1) * char_size,
            char_size)) {
            j = border[j];
        }
        
        if (mxbm_chars_equal(
            (const char*)pattern + i * char_size,
            (const char*)pattern + (j + 1) * char_size,
            char_size)) {
            border[i] = j + 1;
        } else {
            border[i] = -1;
        }
    }
}

static void mxbm_build_good_suffix_table(const void *pattern, Py_ssize_t pattern_len, 
                                         Py_ssize_t *table, int char_size) {
    /* Very conservative good suffix implementation */
    
    /* Initialize all entries to 1 (minimal safe shift that never overshoots) */
    for (Py_ssize_t i = 0; i < pattern_len; i++) {
        table[i] = 1;
    }
    
    /* This ensures we never skip potential matches */
    /* The bad character heuristic will still provide speedup */
}

/* ========================================================================= */
/* 1-BYTE BOYER-MOORE (Classic dense table approach) */
/* ========================================================================= */

int mxbm_init_1byte(mxbm_context_1byte *ctx, const mxbm_char_1byte *pattern, Py_ssize_t pattern_len) {
    if (!ctx || !pattern || pattern_len < MXBM_MIN_PATTERN_LEN || pattern_len > MXBM_MAX_PATTERN_LEN) {
        return -1;
    }
    
    ctx->pattern = pattern;
    ctx->pattern_len = pattern_len;
    
    /* Initialize bad character table - classic dense approach */
    for (int i = 0; i < 256; i++) {
        ctx->bad_char_table[i] = (int)pattern_len;
    }
    
    /* Fill bad character table with actual character positions */
    for (Py_ssize_t i = 0; i < pattern_len - 1; i++) {
        ctx->bad_char_table[pattern[i]] = (int)(pattern_len - 1 - i);
    }
    
    /* Build good suffix table */
    ctx->good_suffix_table = (Py_ssize_t *)PyMem_Malloc(pattern_len * sizeof(Py_ssize_t));
    if (!ctx->good_suffix_table) {
        return -1;
    }
    
    mxbm_build_good_suffix_table(pattern, pattern_len, ctx->good_suffix_table, 1);
    
    
    return 0;
}

void mxbm_cleanup_1byte(mxbm_context_1byte *ctx) {
    if (ctx && ctx->good_suffix_table) {
        PyMem_Free(ctx->good_suffix_table);
        ctx->good_suffix_table = NULL;
    }
}

Py_ssize_t mxbm_search_1byte(const mxbm_context_1byte *ctx, 
                             const mxbm_char_1byte *text, 
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos) {
    if (!ctx || !text || start_pos < 0 || start_pos >= text_len) {
        return -1;
    }
    
    const Py_ssize_t pattern_len = ctx->pattern_len;
    if (pattern_len > text_len - start_pos) {
        return -1;
    }
    
    const mxbm_char_1byte *pattern = ctx->pattern;
    Py_ssize_t pos = start_pos + pattern_len - 1;
    
    while (pos < text_len) {
        Py_ssize_t pattern_idx = pattern_len - 1;
        Py_ssize_t text_idx = pos;
        
        /* Match from right to left */
        while (pattern_idx >= 0 && pattern[pattern_idx] == text[text_idx]) {
            pattern_idx--;
            text_idx--;
        }
        
        if (pattern_idx < 0) {
            /* Found match */
            return text_idx + 1;
        }
        
        /* Calculate shifts using both bad character and good suffix rules */
        int bad_char_shift = ctx->bad_char_table[text[text_idx]];
        Py_ssize_t good_suffix_shift = ctx->good_suffix_table[pattern_idx];
        
        
        pos += MXBM_MAX(bad_char_shift, good_suffix_shift);
    }
    
    return -1; /* Not found */
}

/* ========================================================================= */
/* 2-BYTE BOYER-MOORE (Hash table approach for sparse character space) */
/* ========================================================================= */

int mxbm_init_2byte(mxbm_context_2byte *ctx, const mxbm_char_2byte *pattern, Py_ssize_t pattern_len) {
    if (!ctx || !pattern || pattern_len < MXBM_MIN_PATTERN_LEN || pattern_len > MXBM_MAX_PATTERN_LEN) {
        return -1;
    }
    
    ctx->pattern = pattern;
    ctx->pattern_len = pattern_len;
    ctx->default_shift = (int)pattern_len;
    
    /* Initialize hash table */
    for (int i = 0; i < MXBM_2BYTE_HASH_SIZE; i++) {
        ctx->bad_char_hash[i].ch = 0;
        ctx->bad_char_hash[i].shift = ctx->default_shift;
    }
    
    /* Build bad character hash table */
    for (Py_ssize_t i = 0; i < pattern_len - 1; i++) {
        Py_UCS2 ch = pattern[i];
        int hash_idx = mxbm_hash_2byte(ch);
        int shift = (int)(pattern_len - 1 - i);
        
        /* Handle collisions by keeping the smaller shift (more aggressive) */
        if (ctx->bad_char_hash[hash_idx].ch == 0 || ctx->bad_char_hash[hash_idx].shift > shift) {
            ctx->bad_char_hash[hash_idx].ch = ch;
            ctx->bad_char_hash[hash_idx].shift = shift;
        }
    }
    
    /* Build good suffix table */
    ctx->good_suffix_table = (Py_ssize_t *)PyMem_Malloc(pattern_len * sizeof(Py_ssize_t));
    if (!ctx->good_suffix_table) {
        return -1;
    }
    
    mxbm_build_good_suffix_table(pattern, pattern_len, ctx->good_suffix_table, 2);
    
    return 0;
}

void mxbm_cleanup_2byte(mxbm_context_2byte *ctx) {
    if (ctx && ctx->good_suffix_table) {
        PyMem_Free(ctx->good_suffix_table);
        ctx->good_suffix_table = NULL;
    }
}

static int mxbm_get_bad_char_shift_2byte(const mxbm_context_2byte *ctx, Py_UCS2 ch) {
    int hash_idx = mxbm_hash_2byte(ch);
    const mxbm_hash_entry_2byte *entry = &ctx->bad_char_hash[hash_idx];
    
    if (entry->ch == ch) {
        return entry->shift;
    }
    return ctx->default_shift;
}

Py_ssize_t mxbm_search_2byte(const mxbm_context_2byte *ctx,
                             const mxbm_char_2byte *text,
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos) {
    if (!ctx || !text || start_pos < 0 || start_pos >= text_len) {
        return -1;
    }
    
    const Py_ssize_t pattern_len = ctx->pattern_len;
    if (pattern_len > text_len - start_pos) {
        return -1;
    }
    
    const mxbm_char_2byte *pattern = ctx->pattern;
    Py_ssize_t pos = start_pos + pattern_len - 1;
    
    while (pos < text_len) {
        Py_ssize_t pattern_idx = pattern_len - 1;
        Py_ssize_t text_idx = pos;
        
        /* Match from right to left */
        while (pattern_idx >= 0 && pattern[pattern_idx] == text[text_idx]) {
            pattern_idx--;
            text_idx--;
        }
        
        if (pattern_idx < 0) {
            /* Found match */
            return text_idx + 1;
        }
        
        /* Calculate shifts */
        int bad_char_shift = mxbm_get_bad_char_shift_2byte(ctx, text[text_idx]);
        Py_ssize_t good_suffix_shift = ctx->good_suffix_table[pattern_idx];
        
        pos += MXBM_MAX(bad_char_shift, good_suffix_shift);
    }
    
    return -1; /* Not found */
}

/* ========================================================================= */
/* 4-BYTE BOYER-MOORE (Hybrid cache + fallback approach) */
/* ========================================================================= */

int mxbm_init_4byte(mxbm_context_4byte *ctx, const mxbm_char_4byte *pattern, Py_ssize_t pattern_len) {
    if (!ctx || !pattern || pattern_len < MXBM_MIN_PATTERN_LEN || pattern_len > MXBM_MAX_PATTERN_LEN) {
        return -1;
    }
    
    ctx->pattern = pattern;
    ctx->pattern_len = pattern_len;
    ctx->default_shift = (int)pattern_len;
    ctx->cache_used = 0;
    
    /* Initialize character cache */
    for (int i = 0; i < MXBM_4BYTE_CACHE_SIZE; i++) {
        ctx->char_cache[i].ch = 0;
        ctx->char_cache[i].shift = ctx->default_shift;
    }
    
    /* Build character cache with most recent characters from pattern */
    for (Py_ssize_t i = 0; i < pattern_len - 1 && ctx->cache_used < MXBM_4BYTE_CACHE_SIZE; i++) {
        Py_UCS4 ch = pattern[i];
        int cache_idx = mxbm_cache_index_4byte(ch);
        int shift = (int)(pattern_len - 1 - i);
        
        /* Only cache if slot is empty or we have a better (smaller) shift */
        if (ctx->char_cache[cache_idx].ch == 0 || ctx->char_cache[cache_idx].shift > shift) {
            if (ctx->char_cache[cache_idx].ch == 0) {
                ctx->cache_used++;
            }
            ctx->char_cache[cache_idx].ch = ch;
            ctx->char_cache[cache_idx].shift = shift;
        }
    }
    
    /* Build good suffix table */
    ctx->good_suffix_table = (Py_ssize_t *)PyMem_Malloc(pattern_len * sizeof(Py_ssize_t));
    if (!ctx->good_suffix_table) {
        return -1;
    }
    
    mxbm_build_good_suffix_table(pattern, pattern_len, ctx->good_suffix_table, 4);
    
    return 0;
}

void mxbm_cleanup_4byte(mxbm_context_4byte *ctx) {
    if (ctx && ctx->good_suffix_table) {
        PyMem_Free(ctx->good_suffix_table);
        ctx->good_suffix_table = NULL;
    }
}

static int mxbm_get_bad_char_shift_4byte(const mxbm_context_4byte *ctx, Py_UCS4 ch) {
    int cache_idx = mxbm_cache_index_4byte(ch);
    const mxbm_cache_entry_4byte *entry = &ctx->char_cache[cache_idx];
    
    if (entry->ch == ch) {
        return entry->shift;
    }
    return ctx->default_shift;
}

Py_ssize_t mxbm_search_4byte(const mxbm_context_4byte *ctx,
                             const mxbm_char_4byte *text,
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos) {
    if (!ctx || !text || start_pos < 0 || start_pos >= text_len) {
        return -1;
    }
    
    const Py_ssize_t pattern_len = ctx->pattern_len;
    if (pattern_len > text_len - start_pos) {
        return -1;
    }
    
    const mxbm_char_4byte *pattern = ctx->pattern;
    Py_ssize_t pos = start_pos + pattern_len - 1;
    
    while (pos < text_len) {
        Py_ssize_t pattern_idx = pattern_len - 1;
        Py_ssize_t text_idx = pos;
        
        /* Match from right to left */
        while (pattern_idx >= 0 && pattern[pattern_idx] == text[text_idx]) {
            pattern_idx--;
            text_idx--;
        }
        
        if (pattern_idx < 0) {
            /* Found match */
            return text_idx + 1;
        }
        
        /* Calculate shifts */
        int bad_char_shift = mxbm_get_bad_char_shift_4byte(ctx, text[text_idx]);
        Py_ssize_t good_suffix_shift = ctx->good_suffix_table[pattern_idx];
        
        pos += MXBM_MAX(bad_char_shift, good_suffix_shift);
    }
    
    return -1; /* Not found */
}

/* ========================================================================= */
/* BYTE-STRING BOYER-MOORE (Raw byte search) */
/* ========================================================================= */

int mxbm_init_bytes(mxbm_context_bytes *ctx, const char *pattern, Py_ssize_t pattern_len) {
    if (!ctx || !pattern || pattern_len < MXBM_MIN_PATTERN_LEN || pattern_len > MXBM_MAX_PATTERN_LEN) {
        return -1;
    }
    
    ctx->pattern = pattern;
    ctx->pattern_len = pattern_len;
    
    /* Initialize bad character table */
    for (int i = 0; i < 256; i++) {
        ctx->bad_char_table[i] = (int)pattern_len;
    }
    
    /* Fill bad character table */
    for (Py_ssize_t i = 0; i < pattern_len - 1; i++) {
        unsigned char ch = (unsigned char)pattern[i];
        ctx->bad_char_table[ch] = (int)(pattern_len - 1 - i);
    }
    
    /* Build good suffix table */
    ctx->good_suffix_table = (Py_ssize_t *)PyMem_Malloc(pattern_len * sizeof(Py_ssize_t));
    if (!ctx->good_suffix_table) {
        return -1;
    }
    
    mxbm_build_good_suffix_table(pattern, pattern_len, ctx->good_suffix_table, 1);
    
    return 0;
}

void mxbm_cleanup_bytes(mxbm_context_bytes *ctx) {
    if (ctx && ctx->good_suffix_table) {
        PyMem_Free(ctx->good_suffix_table);
        ctx->good_suffix_table = NULL;
    }
}

Py_ssize_t mxbm_search_bytes(const mxbm_context_bytes *ctx,
                             const char *text,
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos) {
    if (!ctx || !text || start_pos < 0 || start_pos >= text_len) {
        return -1;
    }
    
    const Py_ssize_t pattern_len = ctx->pattern_len;
    if (pattern_len > text_len - start_pos) {
        return -1;
    }
    
    const char *pattern = ctx->pattern;
    Py_ssize_t pos = start_pos + pattern_len - 1;
    
    while (pos < text_len) {
        Py_ssize_t pattern_idx = pattern_len - 1;
        Py_ssize_t text_idx = pos;
        
        /* Match from right to left */
        while (pattern_idx >= 0 && pattern[pattern_idx] == text[text_idx]) {
            pattern_idx--;
            text_idx--;
        }
        
        if (pattern_idx < 0) {
            /* Found match */
            return text_idx + 1;
        }
        
        /* Calculate shifts */
        unsigned char mismatch_char = (unsigned char)text[text_idx];
        int bad_char_shift = ctx->bad_char_table[mismatch_char];
        Py_ssize_t good_suffix_shift = ctx->good_suffix_table[pattern_idx];
        
        pos += MXBM_MAX(bad_char_shift, good_suffix_shift);
    }
    
    return -1; /* Not found */
}

/* ========================================================================= */
/* HIGH-LEVEL UNICODE INTERFACE */
/* ========================================================================= */

int mxbm_init_unicode(mxbm_context_unicode *ctx, PyObject *pattern_obj) {
    if (!ctx || !pattern_obj) {
        return -1;
    }
    
    if (PyBytes_Check(pattern_obj)) {
        /* Byte string */
        const char *pattern_data = PyBytes_AS_STRING(pattern_obj);
        Py_ssize_t pattern_len = PyBytes_GET_SIZE(pattern_obj);
        
        ctx->kind = MXBM_KIND_BYTES;
        return mxbm_init_bytes(&ctx->u.ctx_bytes, pattern_data, pattern_len);
    }
    else if (PyUnicode_Check(pattern_obj)) {
        /* Unicode string */
        if (PyUnicode_READY(pattern_obj) < 0) {
            return -1;
        }
        
        int unicode_kind = PyUnicode_KIND(pattern_obj);
        Py_ssize_t pattern_len = PyUnicode_GET_LENGTH(pattern_obj);
        void *pattern_data = PyUnicode_DATA(pattern_obj);
        
        switch (unicode_kind) {
            case PyUnicode_1BYTE_KIND:
                ctx->kind = MXBM_KIND_1BYTE;
                return mxbm_init_1byte(&ctx->u.ctx_1byte, 
                                      (const mxbm_char_1byte*)pattern_data, 
                                      pattern_len);
                
            case PyUnicode_2BYTE_KIND:
                ctx->kind = MXBM_KIND_2BYTE;
                return mxbm_init_2byte(&ctx->u.ctx_2byte, 
                                      (const mxbm_char_2byte*)pattern_data, 
                                      pattern_len);
                
            case PyUnicode_4BYTE_KIND:
                ctx->kind = MXBM_KIND_4BYTE;
                return mxbm_init_4byte(&ctx->u.ctx_4byte, 
                                      (const mxbm_char_4byte*)pattern_data, 
                                      pattern_len);
                
            default:
                return -1;
        }
    }
    
    return -1; /* Unsupported type */
}

void mxbm_cleanup_unicode(mxbm_context_unicode *ctx) {
    if (!ctx) return;
    
    switch (ctx->kind) {
        case MXBM_KIND_1BYTE:
            mxbm_cleanup_1byte(&ctx->u.ctx_1byte);
            break;
        case MXBM_KIND_2BYTE:
            mxbm_cleanup_2byte(&ctx->u.ctx_2byte);
            break;
        case MXBM_KIND_4BYTE:
            mxbm_cleanup_4byte(&ctx->u.ctx_4byte);
            break;
        case MXBM_KIND_BYTES:
            mxbm_cleanup_bytes(&ctx->u.ctx_bytes);
            break;
        default:
            break;
    }
    ctx->kind = MXBM_KIND_UNKNOWN;
}

Py_ssize_t mxbm_search_unicode(const mxbm_context_unicode *ctx,
                               PyObject *text_obj,
                               Py_ssize_t start_pos,
                               Py_ssize_t end_pos) {
    if (!ctx || !text_obj) {
        return -1;
    }
    
    /* Handle byte strings */
    if (ctx->kind == MXBM_KIND_BYTES && PyBytes_Check(text_obj)) {
        const char *text_data = PyBytes_AS_STRING(text_obj);
        Py_ssize_t text_len = PyBytes_GET_SIZE(text_obj);
        
        if (end_pos < 0 || end_pos > text_len) {
            end_pos = text_len;
        }
        
        return mxbm_search_bytes(&ctx->u.ctx_bytes, text_data, end_pos, start_pos);
    }
    
    /* Handle Unicode strings */
    if (PyUnicode_Check(text_obj)) {
        if (PyUnicode_READY(text_obj) < 0) {
            return -1;
        }
        
        int text_kind = PyUnicode_KIND(text_obj);
        Py_ssize_t text_len = PyUnicode_GET_LENGTH(text_obj);
        void *text_data = PyUnicode_DATA(text_obj);
        
        if (end_pos < 0 || end_pos > text_len) {
            end_pos = text_len;
        }
        
        /* Check if text and pattern have compatible kinds */
        if ((ctx->kind == MXBM_KIND_1BYTE && text_kind == PyUnicode_1BYTE_KIND) ||
            (ctx->kind == MXBM_KIND_2BYTE && text_kind == PyUnicode_2BYTE_KIND) ||
            (ctx->kind == MXBM_KIND_4BYTE && text_kind == PyUnicode_4BYTE_KIND)) {
            
            switch (ctx->kind) {
                case MXBM_KIND_1BYTE:
                    return mxbm_search_1byte(&ctx->u.ctx_1byte,
                                           (const mxbm_char_1byte*)text_data,
                                           end_pos, start_pos);
                case MXBM_KIND_2BYTE:
                    return mxbm_search_2byte(&ctx->u.ctx_2byte,
                                           (const mxbm_char_2byte*)text_data,
                                           end_pos, start_pos);
                case MXBM_KIND_4BYTE:
                    return mxbm_search_4byte(&ctx->u.ctx_4byte,
                                           (const mxbm_char_4byte*)text_data,
                                           end_pos, start_pos);
                default:
                    break;
            }
        }
    }
    
    return -1; /* Incompatible types or error */
}