/* 
  mxte_modern.h -- Modern string handling abstraction layer

  This header provides a unified interface for accessing strings in modern Python
  with support for different character widths (1, 2, and 4 bytes).

  Copyright (c) 2024
*/

#ifndef MXTE_MODERN_H
#define MXTE_MODERN_H

#include "Python.h"
#include "mxpyapi.h"

/* Character type definitions for different string kinds */
typedef unsigned char TE_CHAR_1BYTE;
typedef Py_UCS2 TE_CHAR_2BYTE;
typedef Py_UCS4 TE_CHAR_4BYTE;

/* String kind identifiers */
#define TE_KIND_1BYTE 1
#define TE_KIND_2BYTE 2  
#define TE_KIND_4BYTE 4

/* Determine string kind from Python Unicode object (Python 3.3+ only) */
static inline int mxte_get_string_kind(PyObject *str) {
    if (PyBytes_Check(str)) {
        return TE_KIND_1BYTE;
    }
    if (PyUnicode_Check(str)) {
        if (PyUnicode_READY(str) < 0) {
            return -1;
        }
        switch (PyUnicode_KIND(str)) {
            case PyUnicode_1BYTE_KIND:
                return TE_KIND_1BYTE;
            case PyUnicode_2BYTE_KIND:
                return TE_KIND_2BYTE;
            case PyUnicode_4BYTE_KIND:
                return TE_KIND_4BYTE;
        }
    }
    return -1; /* Unknown type */
}

/* Get string data pointer for given kind (Python 3.3+ only) */
static inline void* mxte_get_string_data(PyObject *str, int kind) {
    if (kind == TE_KIND_1BYTE && PyBytes_Check(str)) {
        return PyBytes_AS_STRING(str);
    }
    if (PyUnicode_Check(str)) {
        if (PyUnicode_READY(str) < 0) {
            return NULL;
        }
        return PyUnicode_DATA(str);
    }
    return NULL;
}

/* Get string length (Python 3.3+ only) */
static inline Py_ssize_t mxte_get_string_length(PyObject *str, int kind) {
    if (kind == TE_KIND_1BYTE && PyBytes_Check(str)) {
        return PyBytes_GET_SIZE(str);
    }
    if (PyUnicode_Check(str)) {
        if (PyUnicode_READY(str) < 0) {
            return -1;
        }
        return PyUnicode_GET_LENGTH(str);
    }
    return -1;
}

/* Create string from data (Python 3.3+ only) */
static inline PyObject* mxte_create_string(const void *data, Py_ssize_t size, int kind) {
    switch (kind) {
        case TE_KIND_1BYTE:
            return PyBytes_FromStringAndSize((const char*)data, size);
        case TE_KIND_2BYTE:
            return PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, data, size);
        case TE_KIND_4BYTE:
            return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, data, size);
        default:
            PyErr_SetString(PyExc_ValueError, "Invalid string kind");
            return NULL;
    }
}

/* Get character at index (generic macro) */
#define MXTE_GET_CHAR(data, index, kind) \
    ((kind) == TE_KIND_1BYTE ? ((TE_CHAR_1BYTE*)(data))[(index)] : \
     (kind) == TE_KIND_2BYTE ? ((TE_CHAR_2BYTE*)(data))[(index)] : \
                               ((TE_CHAR_4BYTE*)(data))[(index)])

/* Set character at index (generic macro) */
#define MXTE_SET_CHAR(data, index, value, kind) do { \
    if ((kind) == TE_KIND_1BYTE) ((TE_CHAR_1BYTE*)(data))[(index)] = (TE_CHAR_1BYTE)(value); \
    else if ((kind) == TE_KIND_2BYTE) ((TE_CHAR_2BYTE*)(data))[(index)] = (TE_CHAR_2BYTE)(value); \
    else ((TE_CHAR_4BYTE*)(data))[(index)] = (TE_CHAR_4BYTE)(value); \
} while(0)

/* Compare memory regions */
static inline int mxte_compare_memory(const void *s1, const void *s2, Py_ssize_t n, int kind) {
    if (kind == TE_KIND_1BYTE) {
        return memcmp(s1, s2, n);
    } else if (kind == TE_KIND_2BYTE) {
        const TE_CHAR_2BYTE *p1 = (const TE_CHAR_2BYTE*)s1;
        const TE_CHAR_2BYTE *p2 = (const TE_CHAR_2BYTE*)s2;
        for (Py_ssize_t i = 0; i < n; i++) {
            if (p1[i] != p2[i]) {
                return p1[i] < p2[i] ? -1 : 1;
            }
        }
        return 0;
    } else {
        const TE_CHAR_4BYTE *p1 = (const TE_CHAR_4BYTE*)s1;
        const TE_CHAR_4BYTE *p2 = (const TE_CHAR_4BYTE*)s2;
        for (Py_ssize_t i = 0; i < n; i++) {
            if (p1[i] != p2[i]) {
                return p1[i] < p2[i] ? -1 : 1;
            }
        }
        return 0;
    }
}

/* Helper functions for eliminating PyUnicode_AS_UNICODE usage in legacy code */

/* Get Unicode data as a temporary Py_UNICODE buffer (for legacy compatibility) */
static inline Py_UNICODE* mxte_get_unicode_data(PyObject *str) {
    if (!PyUnicode_Check(str)) {
        return NULL;
    }
    
    if (PyUnicode_READY(str) < 0) {
        return NULL;
    }
    
    int kind = PyUnicode_KIND(str);
    void *data = PyUnicode_DATA(str);
    Py_ssize_t length = PyUnicode_GET_LENGTH(str);
    
    /* For direct compatibility when sizes match */
    if ((kind == PyUnicode_1BYTE_KIND && sizeof(Py_UNICODE) == 1) ||
        (kind == PyUnicode_2BYTE_KIND && sizeof(Py_UNICODE) == 2) ||
        (kind == PyUnicode_4BYTE_KIND && sizeof(Py_UNICODE) == 4)) {
        return (Py_UNICODE *)data;
    }
    
    /* Convert to Py_UNICODE format for legacy code */
    static Py_UNICODE *converted_buffer = NULL;
    static Py_ssize_t buffer_size = 0;
    
    if (buffer_size < length + 1) {
        PyMem_Free(converted_buffer);
        converted_buffer = (Py_UNICODE *)PyMem_Malloc(sizeof(Py_UNICODE) * (length + 1));
        if (!converted_buffer) {
            buffer_size = 0;
            PyErr_NoMemory();
            return NULL;
        }
        buffer_size = length + 1;
    }
    
    /* Convert data */
    for (Py_ssize_t i = 0; i < length; i++) {
        converted_buffer[i] = (Py_UNICODE)PyUnicode_READ(kind, data, i);
    }
    converted_buffer[length] = 0;
    
    return converted_buffer;
}

/* Create Unicode string from Py_UNICODE array using modern API */
static inline PyObject* mxte_create_unicode_from_unicode(const Py_UNICODE *data, Py_ssize_t length) {
    if (!data) {
        return PyUnicode_New(length, 0);
    }
    
    /* Use modern API to create from Py_UNICODE data */
    if (sizeof(Py_UNICODE) == 2) {
        return PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, data, length);
    } else if (sizeof(Py_UNICODE) == 4) {
        return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, data, length);
    } else {
        PyErr_SetString(PyExc_SystemError, "Unsupported Py_UNICODE size");
        return NULL;
    }
}

/* Get single Unicode character using modern API */
static inline Py_UNICODE mxte_get_unicode_char(PyObject *str, Py_ssize_t index) {
    if (!PyUnicode_Check(str)) {
        return 0;
    }
    
    if (PyUnicode_READY(str) < 0) {
        return 0;
    }
    
    return (Py_UNICODE)PyUnicode_READ(PyUnicode_KIND(str), PyUnicode_DATA(str), index);
}

/* String slice using modern API */
static inline PyObject* mxte_unicode_slice(PyObject *str, Py_ssize_t start, Py_ssize_t length) {
    if (!PyUnicode_Check(str)) {
        return NULL;
    }
    
    if (PyUnicode_READY(str) < 0) {
        return NULL;
    }
    
    return PyUnicode_Substring(str, start, start + length);
}

/* Create substring from buffer offset and length (for legacy pointer arithmetic) */
static inline PyObject* mxte_unicode_substring_from_buffer(PyObject *str, Py_UNICODE *buffer_start, Py_ssize_t buffer_offset, Py_ssize_t length) {
    if (!PyUnicode_Check(str)) {
        return NULL;
    }
    
    /* Calculate the actual start position in the original string */
    Py_UNICODE *str_data = mxte_get_unicode_data(str);
    if (!str_data) {
        return NULL;
    }
    
    Py_ssize_t start = buffer_offset;  /* offset from buffer_start */
    
    return mxte_unicode_slice(str, start, length);
}

/* Modern replacement for PyObject_AsCharBuffer using buffer protocol */
static inline int mxte_get_char_buffer(PyObject *obj, const char **buffer, Py_ssize_t *buffer_len) {
    if (PyBytes_Check(obj)) {
        *buffer = PyBytes_AS_STRING(obj);
        *buffer_len = PyBytes_GET_SIZE(obj);
        return 0;
    }
    
    /* For other objects, try buffer protocol */
    Py_buffer view;
    if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) != 0) {
        return -1;
    }
    
    *buffer = (const char *)view.buf;
    *buffer_len = view.len;
    
    /* Note: We release the buffer immediately, which isn't perfect
       but maintains the same API. Callers should migrate to buffer protocol. */
    PyBuffer_Release(&view);
    return 0;
}

/* Modern replacement for PyUnicode_GET_DATA_SIZE */
static inline Py_ssize_t mxte_get_unicode_data_size(PyObject *obj) {
    if (!PyUnicode_Check(obj)) {
        return -1;
    }
    
    if (PyUnicode_READY(obj) < 0) {
        return -1;
    }
    
    return PyUnicode_GET_LENGTH(obj) * PyUnicode_KIND(obj);
}

#endif /* MXTE_MODERN_H */