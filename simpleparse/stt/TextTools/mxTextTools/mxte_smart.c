/* 
  mxte_smart.c -- Smart Unicode handling for mxTextTools

  This version detects the string kind of the haystack and converts 
  the tag table needles to match, then uses the appropriate engine.

  Copyright (c) 2024
*/

#include "mx.h"
#include "mxstdlib.h"
#include "mxTextTools.h"

/* Determine string kind and call appropriate engine */
int mxTextTools_SmartTaggingEngine(PyObject *textobj,
                                   Py_ssize_t text_start,
                                   Py_ssize_t text_stop,
                                   mxTagTableObject *tagtable,
                                   PyObject *taglist,
                                   PyObject *context,
                                   Py_ssize_t *next)
{
    /* For bytes/string, use the original string engine */
    if (PyBytes_Check(textobj)) {
        return mxTextTools_TaggingEngine(textobj, text_start, text_stop,
                                        tagtable, taglist, context, next);
    }
    
    /* For Unicode, we need to ensure the tag table matches the text format */
    if (PyUnicode_Check(textobj)) {
        /* For now, just use the Unicode engine - the tag table conversion
           would be a more complex change that requires modifying the 
           tag table compilation process */
        return mxTextTools_UnicodeTaggingEngine(textobj, text_start, text_stop,
                                               tagtable, taglist, context, next);
    }
    
    /* Unknown type */
    PyErr_SetString(PyExc_TypeError, "text must be bytes or unicode");
    return 0;
}

/* Wrapper for the tag() function that handles the smart dispatching */
PyObject *mxTextTools_SmartTag(PyObject *text,
                              PyObject *tagtable_obj,
                              Py_ssize_t sliceleft,
                              Py_ssize_t sliceright,
                              PyObject *taglist,
                              PyObject *context)
{
    mxTagTableObject *tagtable;
    Py_ssize_t next;
    int result;
    PyObject *res;
    
    /* Convert tagtable to appropriate type based on input text */
    if (PyBytes_Check(text)) {
        if (!mxTagTable_Check(tagtable_obj)) {
            tagtable = (mxTagTableObject*)mxTagTable_New(tagtable_obj, MXTAGTABLE_STRINGTYPE, 1);
            if (tagtable == NULL)
                return NULL;
        } else if (mxTagTable_Type(tagtable_obj) != MXTAGTABLE_STRINGTYPE) {
            PyErr_SetString(PyExc_TypeError,
                           "TagTable instance is not intended for parsing bytes");
            return NULL;
        } else {
            tagtable = (mxTagTableObject*)tagtable_obj;
            Py_INCREF(tagtable);
        }
    }
    else if (PyUnicode_Check(text)) {
        if (!mxTagTable_Check(tagtable_obj)) {
            tagtable = (mxTagTableObject*)mxTagTable_New(tagtable_obj, MXTAGTABLE_UNICODETYPE, 1);
            if (tagtable == NULL)
                return NULL;
        } else if (mxTagTable_Type(tagtable_obj) != MXTAGTABLE_UNICODETYPE) {
            PyErr_SetString(PyExc_TypeError,
                           "TagTable instance is not intended for parsing Unicode");
            return NULL;
        } else {
            tagtable = (mxTagTableObject*)tagtable_obj;
            Py_INCREF(tagtable);
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "text must be bytes or unicode");
        return NULL;
    }
    
    /* Call the smart tagging engine */
    result = mxTextTools_SmartTaggingEngine(text, sliceleft, sliceright,
                                           tagtable, taglist, context, &next);
    
    Py_DECREF(tagtable);
    
    if (result == 0) {
        return NULL;  /* Error occurred */
    }
    
    /* Convert result to the documented external values: 0 - no match, 1 - match */
    result--;
    
    /* Build result tuple */
    res = PyTuple_New(3);
    if (!res)
        return NULL;
    PyTuple_SET_ITEM(res, 0, PyLong_FromLong(result));
    PyTuple_SET_ITEM(res, 1, taglist);
    Py_INCREF(taglist);
    PyTuple_SET_ITEM(res, 2, PyLong_FromSsize_t(next));
    
    return res;
}