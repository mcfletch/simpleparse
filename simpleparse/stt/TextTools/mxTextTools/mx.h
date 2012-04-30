#ifndef MX_H
#define MX_H

/* 
  mx -- Marc's eXtension modules for Python: basic macros

  This file is only meant to be included by the extension modules.
  DO NOT include it in the extension module's header file, since it
  will definitely cause troubles then.

  To enable debugging ceratin things, define one of these before
  including this file:

  MAL_REF_DEBUG -- debug reference counts (Py_MY_xxx) [this file]
  MAL_DEBUG     -- enable debug output (DPRINTF) [mxstdlib.h]
  MAL_MEM_DEBUG -- enable malloc output (new,cnew,free,...) [mxstdlib.h]

  Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com
  See the documentation for further copyright information or contact
  the author.

*/

/* --- Platform or compiler specific tweaks ------------------------------- */

/* Add some platform specific symbols to enable work-arounds for the
   static forward declaration of type definitions; note that the GNU C
   compiler does not have this problem.  

   Many thanks to all who have contributed to this list.

*/
#if (!defined(__GNUC__))
# if (defined(NeXT) || defined(sgi) || defined(_AIX) || (defined(__osf__) && defined(__DECC)) || defined(TrueCompaq64) || defined(__VMS))
#  define BAD_STATIC_FORWARD
# endif
#endif

/* Some more tweaks for various platforms. */

/* VMS needs this define. Thanks to Jean-François PIÉRONNE */
#if defined(__VMS)
# define __SC__
#endif

/* xlC on AIX doesn't like the Python work-around for static forwards
   in ANSI mode (default), so we switch on extended mode. Thanks to
   Albert Chin-A-Young */
#if defined(__xlC__)
# pragma langlvl extended
#endif

/* --- Standard header files ---------------------------------------------- */

/* Include the generic mx header file */
#include "mxh.h"

/* Include nearly all Python symbols & definitions */
#include "Python.h"

/* Include other standard stuff */
#include "mxstdlib.h"

/* Include Python backward compatibility stuff */
#include "mxpyapi.h"

/* --- Compiler support --------------------------------------------------- */

/* Support for compilers which don't like static forward declarations
   of arrays; Python 2.3 removed the support for BAD_STATIC_FORWARD
   which is why we now use our own little helpers here. */
#undef staticforward
#undef statichere
#ifdef BAD_STATIC_FORWARD
# define staticforward extern
# define statichere static
#else
# define staticforward static
# define statichere static
#endif 

/* --- Declare macros ----------------------------------------------------- */

#define Py_NONE (Py_INCREF(Py_None),Py_None)

#ifdef MAL_REF_DEBUG
# define printref(x) printf("* refcount for "#x" = %i\n",(long) x->ob_refcnt);
#else
# define printref(x)
#endif

/* --- Error handling ----------------------------------------------------- */

#define Py_Do(x) {if (!(x)) goto onError;}
#define Py_ReturnOnError(errortype,errorstr) {PyErr_SetString(errortype,errorstr);return NULL;}

#define Py_Assert(x,errortype,errorstr) {if (!(x)) {PyErr_SetString(errortype,errorstr);goto onError;}}
#define Py_AssertWithArg(x,errortype,errorstr,a1) {if (!(x)) {PyErr_Format(errortype,errorstr,a1);goto onError;}}
#define Py_AssertWith2Args(x,errortype,errorstr,a1,a2) {if (!(x)) {PyErr_Format(errortype,errorstr,a1,a2);goto onError;}}
#define Py_AssertWith3Args(x,errortype,errorstr,a1,a2,a3) {if (!(x)) {PyErr_Format(errortype,errorstr,a1,a2,a3);goto onError;}}

#define Py_Error(errortype,errorstr) {PyErr_SetString(errortype,errorstr);goto onError;}
#define Py_ErrorWithArg(errortype,errorstr,a1) {PyErr_Format(errortype,errorstr,a1);goto onError;}
#define Py_ErrorWith2Args(errortype,errorstr,a1,a2) {PyErr_Format(errortype,errorstr,a1,a2);goto onError;}
#define Py_ErrorWith3Args(errortype,errorstr,a1,a2,a3) {PyErr_Format(errortype,errorstr,a1,a2,a3);goto onError;}

/* --- Reference counting ------------------------------------------------- */

#ifdef MAL_REF_DEBUG

static void mx_Py_INCREF(PyObject *v,
			 char *name,
			 char *filename,
			 int lineno)
{
    if (!Py_DebugFlag) {
	Py_XINCREF(v);
	return;
    }
    if (!v)
	mxDebugPrintf("[%s:%5i] Py_XINCREF( %-8s == NULL );\n",
		      filename,lineno,name);
    else {
	Py_INCREF(v);;
	mxDebugPrintf("[%s:%5i] Py_XINCREF( %-8s at 0x%x [%s]); "
		      "new refcount = %i\n",
		      filename,lineno,name,(int)v,v->ob_type->tp_name,
		      v->ob_refcnt);
    }
}

static void mx_Py_DECREF(PyObject *v,
			 char *name,
			 char *filename,
			 int lineno) 
{
    if (!Py_DebugFlag) {
	Py_XDECREF(v);
	return;
    }
    if (!v)
	mxDebugPrintf("[%s:%5i] Py_XDECREF( %-8s == NULL );\n",
		      filename,lineno,name);
    else {
	int refcnt = v->ob_refcnt;
	Py_DECREF(v);
	if (refcnt <= 1)
	    mxDebugPrintf("[%s:%5i] Py_XDECREF( %-8s at 0x%x [%s]); "
			  "object deleted\n",
			  filename,lineno,name,(int)v,v->ob_type->tp_name);
	else
	    mxDebugPrintf("[%s:%5i] Py_XDECREF( %-8s at 0x%x [%s]); "
			  "new refcount = %i\n",
			  filename,lineno,name,(int)v,v->ob_type->tp_name,
			  v->ob_refcnt);
    }
}

static void mx_Py_PRINT_REFCOUNT(PyObject *v,
				 char *name,
				 char *filename,
				 int lineno) 
{
    if (!v)
	mxDebugPrintf("[%s:%5i] Py_PRINT_REFCOUNT( %-8s == NULL );\n",
		      filename,lineno,name);
    else {
	mxDebugPrintf("[%s:%5i] Py_PRINT_REFCOUNT( %-8s at 0x%x [%s]) = %i;\n",
		      filename,lineno,name,(int)v,v->ob_type->tp_name,
		      v->ob_refcnt);
    }
}

# undef Py_INCREF
# define Py_INCREF(x) mx_Py_INCREF((PyObject *)x,#x,__FILE__,__LINE__)
# undef Py_DECREF
# define Py_DECREF(x) mx_Py_DECREF((PyObject *)x,#x,__FILE__,__LINE__)
# undef Py_XINCREF
# define Py_XINCREF(x) mx_Py_INCREF((PyObject *)x,#x,__FILE__,__LINE__)
# undef Py_XDECREF
# define Py_XDECREF(x) mx_Py_DECREF((PyObject *)x,#x,__FILE__,__LINE__)
# define Py_DELETE(x) {if (x->ob_refcnt > 1) mxDebugPrintf("[%s:%5i] Py_DELETE( "#x" ) WARNING: Refcount = %i > 1\n",__FILE__,__LINE__,(int)x->ob_refcnt);Py_DECREF(x);}
# define Py_PRINT_REFCOUNT(x) mx_Py_PRINT_REFCOUNT((PyObject *)x,#x,__FILE__,__LINE__)
#else
# define Py_DELETE(x) Py_DECREF(x)
# define Py_PRINT_REFCOUNT(x) 
#endif

#define Py_DEC_REF(x) {Py_XDECREF(x); x=0;} /* doing this once too often doesn't hurt */

/* Unreference a Python object. This is only used in Python debug
   builds and needed to keep track of all allocated references. Use in
   object constructors or free list implementations.  */
#ifndef _Py_DEC_REFTOTAL
# ifdef Py_REF_DEBUG
#  define _Py_DEC_REFTOTAL _Py_RefTotal--
# else
#  define _Py_DEC_REFTOTAL
# endif
#endif
#define mxPy_UNREF(x) _Py_DEC_REFTOTAL

/* --- Argument passing and checking -------------------------------------- */

/* No arguments expected; also use Py_MethodListEntryNoArgs() for this
   kind of fct; this check is no longer needed in Python 2.3 and
   later */
#if PY_VERSION_HEX >= 0x02030000
# define Py_NoArgsCheck() {if (0) goto onError;}
#else
# define Py_NoArgsCheck() {if (!PyArg_NoArgs(args)) goto onError;}
#endif

/* For functions with old style args (Py_MethodListEntrySingleArg) */
#define Py_GetArgObject(a) {a = args; if (!a) {PyErr_SetString(PyExc_TypeError,"function/method requires an argument"); goto onError;}}
#define Py_GetSingleArg(format,a1) {if (!PyArg_Parse(args,format,&a1)) goto onError;}

/* For functions with new style args: */
#define Py_GetArg(format,a1) {if (!PyArg_ParseTuple(args,format,&a1)) goto onError;}
#define Py_Get2Args(format,a1,a2) {if (!PyArg_ParseTuple(args,format,&a1,&a2)) goto onError;}
#define Py_Get3Args(format,a1,a2,a3) {if (!PyArg_ParseTuple(args,format,&a1,&a2,&a3)) goto onError;}
#define Py_Get4Args(format,a1,a2,a3,a4) {if (!PyArg_ParseTuple(args,format,&a1,&a2,&a3,&a4)) goto onError;}
#define Py_Get5Args(format,a1,a2,a3,a4,a5) {if (!PyArg_ParseTuple(args,format,&a1,&a2,&a3,&a4,&a5)) goto onError;}
#define Py_Get6Args(format,a1,a2,a3,a4,a5,a6) {if (!PyArg_ParseTuple(args,format,&a1,&a2,&a3,&a4,&a5,&a6)) goto onError;}
#define Py_Get7Args(format,a1,a2,a3,a4,a5,a6,a7) {if (!PyArg_ParseTuple(args,format,&a1,&a2,&a3,&a4,&a5,&a6,&a7)) goto onError;}
#define Py_Get8Args(format,a1,a2,a3,a4,a5,a6,a7,a8) {if (!PyArg_ParseTuple(args,format,&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8)) goto onError;}

/* For functions with keywords -- the first macro parameter must be
   the keywords array given as e.g.

   static char *keywords[] = {"first","second","third", 0};

   with an entry for every argument (in the correct order). The
   functions must be included in the method list using
   Py_MethodWithKeywordsListEntry() and be declared as
   Py_C_Function_WithKeywords().

*/
#define Py_KeywordGetArg(keywords,format,a1) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1)) goto onError;}
#define Py_KeywordGet2Args(keywords,format,a1,a2) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1,&a2)) goto onError;}
#define Py_KeywordGet3Args(keywords,format,a1,a2,a3) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1,&a2,&a3)) goto onError;}
#define Py_KeywordGet4Args(keywords,format,a1,a2,a3,a4) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1,&a2,&a3,&a4)) goto onError;}
#define Py_KeywordGet5Args(keywords,format,a1,a2,a3,a4,a5) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1,&a2,&a3,&a4,&a5)) goto onError;}
#define Py_KeywordGet6Args(keywords,format,a1,a2,a3,a4,a5,a6) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1,&a2,&a3,&a4,&a5,&a6)) goto onError;}
#define Py_KeywordGet7Args(keywords,format,a1,a2,a3,a4,a5,a6,a7) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1,&a2,&a3,&a4,&a5,&a6,&a7)) goto onError;}
#define Py_KeywordGet8Args(keywords,format,a1,a2,a3,a4,a5,a6,a7,a8) {if (!PyArg_ParseTupleAndKeywords(args,kws,format,keywords,&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8)) goto onError;}

/* New style macros fof functions supporting keywords -- the C
   variable names are used as template for the keyword list, i.e. they
   must match the Python keyword parameter names.

   Note that format strings with special parameters (e.g. "#s") are
   not allowed since they would cause the keyword list to be out of
   sync.

   The functions must be included in the method list using
   Py_MethodWithKeywordsListEntry() and be declared as
   Py_C_Function_WithKeywords().

   Example:

    Py_C_Function_WithKeywords(
        myfunction,
	"myfunction(filename,dupkeys=0,filemode=0,sectorsize=512)\n\n"
	"Returns a myobject"
	)
    {
	char *filename;
	int sectorsize = 512;
	int dupkeys = 0;
	int filemode = 0;

	Py_KeywordsGet4Args("s|iii",
			    filename,dupkeys,filemode,sectorsize);

	return (PyObject *)myobject_New(filename,
	                                filemode,
					sectorsize,
					dupkeys);
     onError:
	return NULL;
    }

*/
#define Py_KeywordsGetArg(format,a1) {static char *kwslist[] = {#a1,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1)) goto onError;}
#define Py_KeywordsGet2Args(format,a1,a2) {static char *kwslist[] = {#a1,#a2,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1,&a2)) goto onError;}
#define Py_KeywordsGet3Args(format,a1,a2,a3) {static char *kwslist[] = {#a1,#a2,#a3,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1,&a2,&a3)) goto onError;}
#define Py_KeywordsGet4Args(format,a1,a2,a3,a4) {static char *kwslist[] = {#a1,#a2,#a3,#a4,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1,&a2,&a3,&a4)) goto onError;}
#define Py_KeywordsGet5Args(format,a1,a2,a3,a4,a5) {static char *kwslist[] = {#a1,#a2,#a3,#a4,#a5,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1,&a2,&a3,&a4,&a5)) goto onError;}
#define Py_KeywordsGet6Args(format,a1,a2,a3,a4,a5,a6) {static char *kwslist[] = {#a1,#a2,#a3,#a4,#a5,#a6,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1,&a2,&a3,&a4,&a5,&a6)) goto onError;}
#define Py_KeywordsGet7Args(format,a1,a2,a3,a4,a5,a6,a7) {static char *kwslist[] = {#a1,#a2,#a3,#a4,#a5,#a6,#a7,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1,&a2,&a3,&a4,&a5,&a6,&a7)) goto onError;}
#define Py_KeywordsGet8Args(format,a1,a2,a3,a4,a5,a6,a7,a8) {static char *kwslist[] = {#a1,#a2,#a3,#a4,#a5,#a6,#a7,#a8,NULL}; if (!PyArg_ParseTupleAndKeywords(args,kws,format,kwslist,&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8)) goto onError;}

/* --- Returning values to Python ----------------------------------------- */

/* XXX Don't always work: every time you have an 'O' in the BuildValue format
       string, you need to DECREF the variable *after* the tuple has been
       built !!!
*/

#define Py_ReturnNone() {Py_INCREF(Py_None);return Py_None;}
#define Py_ReturnTrue() {Py_INCREF(Py_True);return Py_True;}
#define Py_ReturnFalse() {Py_INCREF(Py_False);return Py_False;}
#define Py_ReturnArg(format,a1) return Py_BuildValue(format,a1);
#define Py_Return Py_ReturnArg
#define Py_Return2Args(format,a1,a2) return Py_BuildValue(format,a1,a2);
#define Py_Return2 Py_Return2Args
#define Py_Return3Args(format,a1,a2,a3) return Py_BuildValue(format,a1,a2,a3);
#define Py_Return3 Py_Return3Args
#define Py_Return4Args(format,a1,a2,a3) return Py_BuildValue(format,a1,a2,a3,a4);
#define Py_Return5Args(format,a1,a2,a3) return Py_BuildValue(format,a1,a2,a3,a4,a5);
#define Py_Return6Args(format,a1,a2,a3) return Py_BuildValue(format,a1,a2,a3,a4,a5,a6);
#define Py_Return7Args(format,a1,a2,a3) return Py_BuildValue(format,a1,a2,a3,a4,a5,a6,a7);

/* Build values */

#define Py_BuildNone() Py_NONE
#define Py_Build(format,x) Py_BuildValue(format,x)
#define Py_Build2(format,x,y) Py_BuildValue(format,x,y)
#define Py_Build3(format,x,y,z) Py_BuildValue(format,x,y,z)

/* --- Declaring Python builtin functions/methods ------------------------- */

/* Declare C function/method fct, having docstring docstr; may use vargargs */
#define Py_C_Function(fct,docstr) \
        static char fct##_docstring[] = docstr;\
        static PyObject *fct(PyObject *self, PyObject *args)

/* Declare C function/method fct, having keywords keywordsarray and a
   docstring docstr; may use vargargs & keywords */
#define Py_C_Function_WithKeywords(fct,docstr) \
        static char fct##_docstring[] = docstr;\
        static PyObject *fct(PyObject *self, PyObject *args, PyObject *kws)

/* These declare: self -- instance pointer for methods, NULL for functions
                  args -- argument tuple
		  kws  -- keywords dict (if applicable)
   plus as statics:
                  <function name>_docstring -- the docstring as given
		  <function name>_keywords  -- the keyword array as given

   Note: use the Py_GetArg macros for functions without keywords,
             and Py_KeywordGetArg macros for functions with keywords
*/

/* --- Method list entries for builtin functions/methods ------------------ */

/* Add a C function/method cname to the module dict as pyname; no
   doc-string */
#define Py_MethodListEntryAny(pyname,cname) {pyname,(PyCFunction)cname,METH_VARARGS}

/* Add a C function/method cname to the module dict as pyname; the
   function can use varargs */
#define Py_MethodListEntry(pyname,cname) {pyname,(PyCFunction)cname,METH_VARARGS,cname##_docstring}

/* Add a C function/method cname to the module dict as pyname; the
   function takes no args; in Python 2.3 a new flag was added for
   these which implements the no args check in the interpreter
   itself. */
#ifdef METH_NOARGS
# define Py_MethodListEntryNoArgs(pyname,cname) {pyname,(PyCFunction)cname,METH_NOARGS,cname##_docstring}
#else
# define Py_MethodListEntryNoArgs(pyname,cname) {pyname,(PyCFunction)cname,0,cname##_docstring}
#endif

/* Add a C function/method cname to the module dict as pyname; the
   function takes one argument: the object is passed in directly
   (without wrapping it into a tuple first), i.e. don't use
   the Py_GetArg-macros or PyArg_ParseTuple(). */
#define Py_MethodListEntrySingleArg(pyname,cname) {pyname,(PyCFunction)cname,0,cname##_docstring}

/* Add a C function/method that uses keywords to the module dict */
#define Py_MethodWithKeywordsListEntry(pyname,cname) {pyname,(PyCFunction)cname,METH_VARARGS | METH_KEYWORDS,cname##_docstring}


/* --- Sequence slicing --------------------------------------------------- */

/* Check a given slice and apply the usual rules for negative indices */
#define Py_CheckSequenceSlice(len,start,stop) {	\
	    if (stop > len)			\
		stop = len;			\
	    else {				\
		if (stop < 0)			\
		    stop += len;		\
		if (stop < 0)			\
		    stop = 0;			\
	    }					\
	    if (start < 0) {			\
		start += len;			\
		if (start < 0)			\
		    start = 0;			\
	    }					\
	    if (stop < start)			\
		start = stop;			\
	}

/* --- Number macros ------------------------------------------------------ */

/* Test for PyFloat_AsDouble() compatible object */
#define PyFloat_Compatible(obj) \
        (obj->ob_type->tp_as_number->nb_float != NULL)

/* --- Text macros -------------------------------------------------------- */

/* Check a given text slice and apply the usual rules for negative
   indices */
#define Py_CheckBufferSlice(textlen,start,stop) \
        Py_CheckSequenceSlice(textlen,start,stop)

/* Dito for string objects */
#define Py_CheckStringSlice(textobj,start,stop) \
        Py_CheckSequenceSlice(PyString_GET_SIZE(textobj),start,stop)

/* For b/w compatibility */
#define Py_CheckSlice(textobj,start,stop) \
        Py_CheckStringSlice(textobj,start,stop)

/* Dito for Unicode objects */
#ifdef PyUnicode_GET_SIZE
# define Py_CheckUnicodeSlice(unicode,start,stop) \
         Py_CheckSequenceSlice(PyUnicode_GET_SIZE(unicode),start,stop)
#endif

/* This assumes that fixed is a constant char array; the strcmp
   function is only called in case the attribute name length exceeds
   10 characters and the first 10 characters match; optimizing
   compilers should eliminate any unused parts of this comparison
   automatically. 

   Note: The latest egcs compiler warns about the subscripts being out
   of range for shorter fixed strings; since no code is generated for
   those comparisons, these warning can safely be ignored. Still, they
   are annoying. See the Py_StringsCompareEqual() macro below for a
   way to work around this.

*/
#define Py_StringsCompareEqualEx(var,fixed,fixedsize)			\
     (var[0] == fixed[0] &&						\
      (fixed[0] == 0 ||							\
       (fixedsize >= 1 && (var[1] == fixed[1] &&			\
	(fixed[1] == 0 ||						\
	 (fixedsize >= 2 && (var[2] == fixed[2] &&			\
	  (fixed[2] == 0 ||						\
	   (fixedsize >= 3 && (var[3] == fixed[3] &&			\
	    (fixed[3] == 0 ||					       	\
	     (fixedsize >= 4 && (var[4] == fixed[4] &&			\
	      (fixed[4] == 0 ||						\
	       (fixedsize >= 5 && (var[5] == fixed[5] &&		\
		(fixed[5] == 0 ||					\
		 (fixedsize >= 6 && (var[6] == fixed[6] &&		\
		  (fixed[6] == 0 ||					\
		   (fixedsize >= 7 && (var[7] == fixed[7] &&		\
		    (fixed[7] == 0 ||					\
		     (fixedsize >= 8 && (var[8] == fixed[8] &&		\
		      (fixed[8] == 0 ||					\
		       (fixedsize >= 9 && (var[9] == fixed[9] &&	\
			(fixed[9] == 0 ||				\
			 (fixedsize >= 10 &&				\
			  strcmp(&var[10],&fixed[10]) == 0		\
			 ))))))))))))))))))))))))))))))

/* This assumes that fixed is a constant char array. 

   The appended string snippet is to shut up the warnings produced by
   newer egcs/gcc compilers about offsets being outside bounds.  

   Note that some compilers do the inlining by themselves or don't
   like the above trick (OpenVMS is one such platform). For these we
   simply use the standard way.

*/

#ifndef __VMS
# define Py_StringsCompareEqual(var,fixed)				\
     Py_StringsCompareEqualEx(var,fixed"\0\0\0\0\0\0\0\0\0\0",sizeof(fixed))
#else
# define Py_StringsCompareEqual(var,fixed) (strcmp(var, fixed) == 0)
#endif

/* Fast character set member check; set must be a "static unsigned
   *char set" array of exactly 32 bytes length generated with
   TextTools.set() */
#define Py_CharInSet(chr,set)					\
        (((unsigned char)(set)[(unsigned char)(chr) >> 3] & 	\
	  (1 << ((unsigned char)(chr) & 7))) != 0)

/* --- Macros for getattr ------------------------------------------------- */

/* Compares var to name and returns 1 iff they match.

   This assumes that name is a constant char array. */

#define Py_WantAttr(var,name) Py_StringsCompareEqual(var,name)

/* --- Module init helpers ------------------------------------------------ */

/* Helper for startup type object initialization */

#define PyType_Init(x)						\
{								\
    x.ob_type = &PyType_Type; 					\
    Py_Assert(x.tp_basicsize >= (int)sizeof(PyObject),	        \
	      PyExc_SystemError,				\
	      "Internal error: tp_basicsize of "#x" too small");\
}

/* Error reporting for module init functions */

#define Py_ReportModuleInitError(modname) {			\
    PyObject *exc_type, *exc_value, *exc_tb;			\
    PyObject *str_type, *str_value;				\
								\
    /* Fetch error objects and convert them to strings */	\
    PyErr_Fetch(&exc_type, &exc_value, &exc_tb);		\
    if (exc_type && exc_value) {				\
	str_type = PyObject_Str(exc_type);			\
	str_value = PyObject_Str(exc_value);			\
    }								\
    else {							\
	str_type = NULL;					\
	str_value = NULL;					\
    }								\
    /* Try to format a more informative error message using the	\
       original error */					\
    if (str_type && str_value &&				\
	PyString_Check(str_type) && PyString_Check(str_value))	\
	PyErr_Format(						\
		PyExc_ImportError,				\
		"initialization of module "modname" failed "	\
		"(%s:%s)",					\
		PyString_AS_STRING(str_type),			\
		PyString_AS_STRING(str_value));			\
    else							\
	PyErr_SetString(					\
		PyExc_ImportError,				\
		"initialization of module "modname" failed");	\
    Py_XDECREF(str_type);					\
    Py_XDECREF(str_value);					\
    Py_XDECREF(exc_type);					\
    Py_XDECREF(exc_value);					\
    Py_XDECREF(exc_tb);						\
}

/* --- SWIG addons -------------------------------------------------------- */

/* Throw this error after having set the correct Python exception
   using e.g. PyErr_SetString(); */
#define mxSWIGError "mxSWIGError"

/* EOF */
#endif

