/*
  mxbm_modern.h -- Modern Boyer-Moore String Search Implementation
  
  A clean, efficient Boyer-Moore implementation designed for SimpleParse.
  Supports 1-byte, 2-byte, and 4-byte character widths with different
  optimization strategies for each.
  
  Copyright (c) 2024 SimpleParse Project
  License: MIT License
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
*/

#ifndef MXBM_MODERN_H
#define MXBM_MODERN_H

#include "Python.h"
#include "mxpyapi.h"

/* Forward declarations for different character types */
typedef unsigned char mxbm_char_1byte;
typedef Py_UCS2 mxbm_char_2byte;
typedef Py_UCS4 mxbm_char_4byte;

/* Boyer-Moore search context structures for different character widths */

/* 1-byte Boyer-Moore: Classic dense jump table (256 entries) */
typedef struct {
    const mxbm_char_1byte *pattern;
    Py_ssize_t pattern_len;
    int bad_char_table[256];        /* Dense table for 1-byte chars */
    Py_ssize_t *good_suffix_table;  /* Good suffix shift table */
} mxbm_context_1byte;

/* 2-byte Boyer-Moore: Hash table approach for sparse character space */
#define MXBM_2BYTE_HASH_SIZE 1024
typedef struct {
    Py_UCS2 ch;
    int shift;
} mxbm_hash_entry_2byte;

typedef struct {
    const mxbm_char_2byte *pattern;
    Py_ssize_t pattern_len;
    int default_shift;              /* Default shift for unmapped characters */
    mxbm_hash_entry_2byte bad_char_hash[MXBM_2BYTE_HASH_SIZE];
    Py_ssize_t *good_suffix_table;  /* Good suffix shift table */
} mxbm_context_2byte;

/* 4-byte Boyer-Moore: Hybrid approach with common chars cached + fallback */
#define MXBM_4BYTE_CACHE_SIZE 256
typedef struct {
    Py_UCS4 ch;
    int shift;
} mxbm_cache_entry_4byte;

typedef struct {
    const mxbm_char_4byte *pattern;
    Py_ssize_t pattern_len;
    int default_shift;              /* Default shift for unmapped characters */
    mxbm_cache_entry_4byte char_cache[MXBM_4BYTE_CACHE_SIZE];  /* Cache for common chars */
    int cache_used;                 /* Number of cache entries used */
    Py_ssize_t *good_suffix_table;  /* Good suffix shift table */
} mxbm_context_4byte;

/* Byte-string Boyer-Moore: For raw byte data (non-Unicode) */
typedef struct {
    const char *pattern;
    Py_ssize_t pattern_len;
    int bad_char_table[256];        /* Dense table for bytes */
    Py_ssize_t *good_suffix_table;  /* Good suffix shift table */
} mxbm_context_bytes;

/* Function declarations */

/* 1-byte Unicode Boyer-Moore */
int mxbm_init_1byte(mxbm_context_1byte *ctx, const mxbm_char_1byte *pattern, Py_ssize_t pattern_len);
void mxbm_cleanup_1byte(mxbm_context_1byte *ctx);
Py_ssize_t mxbm_search_1byte(const mxbm_context_1byte *ctx, 
                             const mxbm_char_1byte *text, 
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos);

/* 2-byte Unicode Boyer-Moore */
int mxbm_init_2byte(mxbm_context_2byte *ctx, const mxbm_char_2byte *pattern, Py_ssize_t pattern_len);
void mxbm_cleanup_2byte(mxbm_context_2byte *ctx);
Py_ssize_t mxbm_search_2byte(const mxbm_context_2byte *ctx,
                             const mxbm_char_2byte *text,
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos);

/* 4-byte Unicode Boyer-Moore */
int mxbm_init_4byte(mxbm_context_4byte *ctx, const mxbm_char_4byte *pattern, Py_ssize_t pattern_len);
void mxbm_cleanup_4byte(mxbm_context_4byte *ctx);
Py_ssize_t mxbm_search_4byte(const mxbm_context_4byte *ctx,
                             const mxbm_char_4byte *text,
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos);

/* Byte-string Boyer-Moore */
int mxbm_init_bytes(mxbm_context_bytes *ctx, const char *pattern, Py_ssize_t pattern_len);
void mxbm_cleanup_bytes(mxbm_context_bytes *ctx);
Py_ssize_t mxbm_search_bytes(const mxbm_context_bytes *ctx,
                             const char *text,
                             Py_ssize_t text_len,
                             Py_ssize_t start_pos);

/* High-level Unicode Boyer-Moore dispatcher */
typedef enum {
    MXBM_KIND_UNKNOWN = 0,
    MXBM_KIND_1BYTE = 1,
    MXBM_KIND_2BYTE = 2,
    MXBM_KIND_4BYTE = 4,
    MXBM_KIND_BYTES = 255
} mxbm_kind_t;

typedef struct {
    mxbm_kind_t kind;
    union {
        mxbm_context_1byte ctx_1byte;
        mxbm_context_2byte ctx_2byte;
        mxbm_context_4byte ctx_4byte;
        mxbm_context_bytes ctx_bytes;
    } u;
} mxbm_context_unicode;

/* High-level Unicode interface */
int mxbm_init_unicode(mxbm_context_unicode *ctx, PyObject *pattern_obj);
void mxbm_cleanup_unicode(mxbm_context_unicode *ctx);
Py_ssize_t mxbm_search_unicode(const mxbm_context_unicode *ctx,
                               PyObject *text_obj,
                               Py_ssize_t start_pos,
                               Py_ssize_t end_pos);

/* Utility functions */
static inline int mxbm_hash_2byte(Py_UCS2 ch) {
    return ((ch * 2654435761U) >> 22) & (MXBM_2BYTE_HASH_SIZE - 1);
}

static inline int mxbm_cache_index_4byte(Py_UCS4 ch) {
    return ch % MXBM_4BYTE_CACHE_SIZE;
}

/* Performance optimization macros */
#define MXBM_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MXBM_MIN(a, b) ((a) < (b) ? (a) : (b))

/* Configuration constants */
#define MXBM_MIN_PATTERN_LEN 2      /* Minimum pattern length for Boyer-Moore */
#define MXBM_MAX_PATTERN_LEN 65536  /* Maximum reasonable pattern length */

#endif /* MXBM_MODERN_H */