/* 
  mxTextTools_modern.c -- Modern-only wrapper for mxTextTools

  This file provides the module interface using only modern Unicode APIs
  for Python 3.3+. It routes all calls to the modern engine implementation.

  Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com
  Copyright (c) 2003-2006, Mike Fletcher; mailto:mcfletch@vrplumber.com
  Copyright (c) 2024, Modern implementation
*/

/* We want all our symbols to be exported */
#ifndef MX_BUILDING_MXTEXTTOOLS
#define MX_BUILDING_MXTEXTTOOLS
#endif

/* Logging file used by debugging facility */
#ifndef MAL_DEBUG_OUTPUTFILE
# define MAL_DEBUG_OUTPUTFILE "mxTextTools.log"
#endif

#include "mx.h"
#include "mxTextTools.h"
#include "mxte_modern.h"
#include "structmember.h"
#include <ctype.h>

#define VERSION "2.1.0"

/* Initial list size used by e.g. setsplit(), setsplitx(),... */
#define INITIAL_LIST_SIZE 64

/* Maximum TagTable cache size. If this limit is reached, the cache
   is cleared to make room for new compile TagTables. */
#define MAX_TAGTABLES_CACHE_SIZE 100

/* Define this to enable the copy-protocol (__copy__, __deepcopy__) */
#define COPY_PROTOCOL

/* Convenience macro for reducing clutter */
#define ADD_INT_CONSTANT(name, value) \
    if (PyModule_AddIntConstant(module, name, value) < 0) \
        return NULL;

/* --- module doc-string -------------------------------------------------- */

PyDoc_STRVAR(Module_docstring,

 MXTEXTTOOLS_MODULE" -- Tools for fast text processing. Version "VERSION" (Modern)\n\n"

 "Copyright (c) 1997-2000, Marc-Andre Lemburg; mailto:mal@lemburg.com\n"
 "Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com\n\n"
 "Copyright (c) 2003-2006, Mike Fletcher; mailto:mcfletch@vrplumber.com\n\n"
 "Copyright (c) 2024, Modern implementation for Python 3.3+\n\n"

 "                 All Rights Reserved\n\n"
 "See the documentation for further information on copyrights,\n"
 "or contact the author.")
;

/* --- module globals ----------------------------------------------------- */

/* Translation strings for the 8-bit versions of lower() and upper() */
static PyObject *mx_ToUpper;
static PyObject *mx_ToLower;

static PyObject *mxTextTools_Error;    /* mxTextTools specific error */
static PyObject *mxTextTools_TagTables;    /* TagTable cache dictionary */

/* Flag telling us whether the module was initialized or not. */
static int mxTextTools_Initialized = 0;

/* --- forward declarations ----------------------------------------------- */

/* --- module helper ------------------------------------------------------ */

static
PyObject *mxTextTools_ToUpper(void)
{
    char tr[256];
    Py_ssize_t i;
    
    for (i = 0; i < 256; i++)
    tr[i] = toupper((char)i);
    return PyBytes_FromStringAndSize(tr, sizeof(tr));
}

static
PyObject *mxTextTools_ToLower(void)
{
    char tr[256];
    Py_ssize_t i;
    
    for (i = 0; i < 256; i++)
    tr[i] = tolower((char)i);
    return PyBytes_FromStringAndSize(tr, sizeof(tr));
}

/* --- module cleanup ----------------------------------------------------- */

static void mxTextToolsModule_Cleanup(void)
{
    /* Clear tag table cache */
    if (mxTextTools_TagTables) {
        PyDict_Clear(mxTextTools_TagTables);
    }
    
    mxTextTools_Initialized = 0;
}

/* --- module method implementations -------------------------------------- */

/* Forward declarations for functions defined in the modern engine */
extern PyObject *mxTextTools_tag_modern(PyObject *module, PyObject *args, PyObject *keywds);
extern PyObject *mxTextTools_join_modern(PyObject *module, PyObject *args);
extern PyObject *mxTextTools_cmp_modern(PyObject *module, PyObject *args);
extern PyObject *mxTextTools_joinlist_modern(PyObject *module, PyObject *args);
extern PyObject *mxTextTools_set_modern(PyObject *module, PyObject *args);
extern PyObject *mxTextTools_setfind_modern(PyObject *module, PyObject *args);
extern PyObject *mxTextTools_setsplit_modern(PyObject *module, PyObject *args);
extern PyObject *mxTextTools_setsplitx_modern(PyObject *module, PyObject *args);
extern PyObject *mxTextTools_setstrip_modern(PyObject *module, PyObject *args);

/* Use the modern implementations for all methods */
#define mxTextTools_tag mxTextTools_tag_modern
#define mxTextTools_join mxTextTools_join_modern
#define mxTextTools_cmp mxTextTools_cmp_modern
#define mxTextTools_joinlist mxTextTools_joinlist_modern
#define mxTextTools_set mxTextTools_set_modern
#define mxTextTools_setfind mxTextTools_setfind_modern
#define mxTextTools_setsplit mxTextTools_setsplit_modern
#define mxTextTools_setsplitx mxTextTools_setsplitx_modern
#define mxTextTools_setstrip mxTextTools_setstrip_modern

/* --- module methods ----------------------------------------------------- */

static PyMethodDef Module_methods[] =
{   
    {"tag", (PyCFunction)mxTextTools_tag, METH_VARARGS|METH_KEYWORDS, "tag(text, table, start=0, ...)"},
    {"join", mxTextTools_join, METH_VARARGS, "join(list, separator='')"},
    {"cmp", mxTextTools_cmp, METH_VARARGS, "cmp(str1, str2)"},
    {"joinlist", mxTextTools_joinlist, METH_VARARGS, "joinlist(list)"},
    {"set", mxTextTools_set, METH_VARARGS, "set(string, [strip_quotes=0])"},
    {"setfind", mxTextTools_setfind, METH_VARARGS, "setfind(string, set)"},
    {"setsplit", mxTextTools_setsplit, METH_VARARGS, "setsplit(string, set)"},
    {"setsplitx", mxTextTools_setsplitx, METH_VARARGS, "setsplitx(string, set)"},
    {"setstrip", mxTextTools_setstrip, METH_VARARGS, "setstrip(string, set)"},
    {"TextSearch", (PyCFunction)mxTextSearch_TextSearch, METH_VARARGS|METH_KEYWORDS, "TextSearch(match, [translate])"},
    {"CharSet", mxCharSet_CharSet, METH_VARARGS, "CharSet(definition)"},
    {"TagTable", mxTagTable_TagTable, METH_VARARGS, "TagTable(table, [cache=1])"},
    {"UnicodeTagTable", mxTagTable_UnicodeTagTable, METH_VARARGS, "UnicodeTagTable(table, [cache=1])"},
    {NULL, NULL} /* sentinel */
};

/* --- module definition -------------------------------------------------- */

static struct PyModuleDef mxTextTools_ModuleDef = {
    PyModuleDef_HEAD_INIT,
    MXTEXTTOOLS_MODULE,     /* m_name */
    Module_docstring,       /* m_doc */
    -1,                     /* m_size */
    Module_methods,         /* m_methods */
    NULL,                   /* m_reload */
    NULL,                   /* m_traverse */
    NULL,                   /* m_clear */
    NULL                    /* m_free */
};

/* --- module initialization ---------------------------------------------- */

static PyObject* mxTextToolsModule_Initialize(void)
{
    PyObject *module;

    if (mxTextTools_Initialized) {
        PyErr_SetString(PyExc_SystemError,
                "can't initialize "MXTEXTTOOLS_MODULE" more than once");
        return NULL;
    }

    /* Only support Python 3.3+ */
    if (PY_VERSION_HEX < 0x03030000) {
        PyErr_SetString(PyExc_RuntimeError,
                "mxTextTools modern implementation requires Python 3.3 or later");
        return NULL;
    }

    /* Init type objects */
    if (PyType_Ready(&mxTextSearch_Type) < 0)
        return NULL;
    if (PyType_Ready(&mxCharSet_Type) < 0)
        return NULL;
    if (PyType_Ready(&mxTagTable_Type) < 0)
        return NULL;

    /* create module */
    module = PyModule_Create(&mxTextTools_ModuleDef);
    if (!module)
        return NULL;

    /* Init TagTable cache */
    mxTextTools_TagTables = PyDict_New();
    if (!mxTextTools_TagTables)
        return NULL;

    /* Register cleanup function */
    if (Py_AtExit(mxTextToolsModule_Cleanup) < 0)
        return NULL;

    /* Add some symbolic constants to the module */
    if (PyModule_AddStringConstant(module, "__version__", VERSION) < 0)
        return NULL;
    mx_ToUpper = mxTextTools_ToUpper();
    if (!mx_ToUpper)
        return NULL;
    if (PyModule_AddObject(module, "to_upper", mx_ToUpper) < 0)
        return NULL;
    mx_ToLower = mxTextTools_ToLower();
    if (!mx_ToLower)
        return NULL;
    if (PyModule_AddObject(module, "to_lower", mx_ToLower) < 0)
        return NULL;

    /* Let the tag table cache live in the module dictionary; we just
       keep a weak reference in mxTextTools_TagTables around. */
    if (PyModule_AddObject(module, "tagtable_cache", mxTextTools_TagTables) < 0)
        return NULL;
    Py_DECREF(mxTextTools_TagTables);

    ADD_INT_CONSTANT("BOYERMOORE", MXTEXTSEARCH_BOYERMOORE);
    ADD_INT_CONSTANT("FASTSEARCH", MXTEXTSEARCH_FASTSEARCH);
    ADD_INT_CONSTANT("TRIVIAL", MXTEXTSEARCH_TRIVIAL);

    /* Init exceptions */
    mxTextTools_Error = PyErr_NewException("mxTextTools.Error", PyExc_Exception, NULL);
    if (!mxTextTools_Error)
        return NULL;
    if (PyModule_AddObject(module, "Error", mxTextTools_Error) < 0)
        return NULL;

    /* Type objects */
    Py_INCREF(&mxTextSearch_Type);
    if (PyModule_AddObject(module, "TextSearchType", (PyObject*) &mxTextSearch_Type) < 0)
        return NULL;
    Py_INCREF(&mxCharSet_Type);
    if (PyModule_AddObject(module, "CharSetType", (PyObject*) &mxCharSet_Type) < 0)
        return NULL;
    Py_INCREF(&mxTagTable_Type);
    if (PyModule_AddObject(module, "TagTableType", (PyObject*) &mxTagTable_Type) < 0)
        return NULL;

    /* Tag Table command symbols (these will be exposed via
       simpleparse.stt.TextTools.Constants.TagTables) */
    ADD_INT_CONSTANT("_const_AllIn", MATCH_ALLIN);
    ADD_INT_CONSTANT("_const_AllNotIn", MATCH_ALLNOTIN);
    ADD_INT_CONSTANT("_const_Is", MATCH_IS);
    ADD_INT_CONSTANT("_const_IsIn", MATCH_ISIN);
    ADD_INT_CONSTANT("_const_IsNot", MATCH_ISNOTIN);
    ADD_INT_CONSTANT("_const_IsNotIn", MATCH_ISNOTIN);

    ADD_INT_CONSTANT("_const_Word", MATCH_WORD);
    ADD_INT_CONSTANT("_const_WordStart", MATCH_WORDSTART);
    ADD_INT_CONSTANT("_const_WordEnd", MATCH_WORDEND);

    ADD_INT_CONSTANT("_const_AllInSet", MATCH_ALLINSET);
    ADD_INT_CONSTANT("_const_IsInSet", MATCH_ISINSET);
    ADD_INT_CONSTANT("_const_AllInCharSet", MATCH_ALLINCHARSET);
    ADD_INT_CONSTANT("_const_IsInCharSet", MATCH_ISINCHARSET);

    ADD_INT_CONSTANT("_const_Fail", MATCH_FAIL);
    ADD_INT_CONSTANT("_const_Jump", MATCH_JUMP);
    ADD_INT_CONSTANT("_const_EOF", MATCH_EOF);
    ADD_INT_CONSTANT("_const_Skip", MATCH_SKIP);
    ADD_INT_CONSTANT("_const_Move", MATCH_MOVE);

    ADD_INT_CONSTANT("_const_JumpTarget", MATCH_JUMPTARGET);

    ADD_INT_CONSTANT("_const_sWordStart", MATCH_SWORDSTART);
    ADD_INT_CONSTANT("_const_sWordEnd", MATCH_SWORDEND);
    ADD_INT_CONSTANT("_const_sFindWord", MATCH_SFINDWORD);
    ADD_INT_CONSTANT("_const_NoWord", MATCH_NOWORD);

    ADD_INT_CONSTANT("_const_Call", MATCH_CALL);
    ADD_INT_CONSTANT("_const_CallArg", MATCH_CALLARG);

    ADD_INT_CONSTANT("_const_Table", MATCH_TABLE);
    ADD_INT_CONSTANT("_const_SubTable", MATCH_SUBTABLE);
    ADD_INT_CONSTANT("_const_TableInList", MATCH_TABLEINLIST);
    ADD_INT_CONSTANT("_const_SubTableInList", MATCH_SUBTABLEINLIST);

    ADD_INT_CONSTANT("_const_Loop", MATCH_LOOP);
    ADD_INT_CONSTANT("_const_LoopControl", MATCH_LOOPCONTROL);

    /* Tag Table command flags */
    ADD_INT_CONSTANT("_const_CallTag", MATCH_CALLTAG);
    ADD_INT_CONSTANT("_const_AppendToTagobj", MATCH_APPENDTAG);
    ADD_INT_CONSTANT("_const_AppendTagobj", MATCH_APPENDTAGOBJ);
    ADD_INT_CONSTANT("_const_AppendMatch", MATCH_APPENDMATCH);
    ADD_INT_CONSTANT("_const_LookAhead", MATCH_LOOKAHEAD);

    /* Tag Table argument integers */
    ADD_INT_CONSTANT("_const_To", MATCH_JUMP_TO);
    ADD_INT_CONSTANT("_const_MatchOk", MATCH_JUMP_MATCHOK);
    ADD_INT_CONSTANT("_const_MatchFail", MATCH_JUMP_MATCHFAIL);
    ADD_INT_CONSTANT("_const_ToEOF", MATCH_MOVE_EOF);
    ADD_INT_CONSTANT("_const_ToBOF", MATCH_MOVE_BOF);
    ADD_INT_CONSTANT("_const_Here", MATCH_FAIL_HERE);

    ADD_INT_CONSTANT("_const_ThisTable", MATCH_THISTABLE);

    ADD_INT_CONSTANT("_const_Break", MATCH_LOOPCONTROL_BREAK);
    ADD_INT_CONSTANT("_const_Reset", MATCH_LOOPCONTROL_RESET);

    DPRINTF("sizeof(string_charset)=%i bytes\n", sizeof(string_charset));
    DPRINTF("sizeof(unicode_charset)=%i bytes\n", sizeof(unicode_charset));

    /* We are now initialized */
    mxTextTools_Initialized = 1;

    return module;
}

/* --- module entry point ------------------------------------------------ */

PyMODINIT_FUNC PyInit_mxTextTools(void)
{
    return mxTextToolsModule_Initialize();
}