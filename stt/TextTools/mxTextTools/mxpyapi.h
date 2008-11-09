#ifndef MXPYAPI_H
#define MXPYAPI_H

/* mxpyapi.h

   This header file includes some new APIs that are not available in
   older API versions, yet are used by the mx-Extensions.

   Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
   Copyright (c) 2000-2003, eGenix.com Software GmbH; mailto:info@egenix.com

*/

/* Emulate PY_VERSION_HEX for older Python versions. */

#ifndef PY_VERSION_HEX
# if PYTHON_API_VERSION == 1007
#  define PY_VERSION_HEX 0x010500F0
# endif
# if PYTHON_API_VERSION == 1006
#  define PY_VERSION_HEX 0x010400F0
# endif
# if PYTHON_API_VERSION < 1006
#  define PY_VERSION_HEX 0
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
  ----------------------------------------------------------------
  Python 2.3 and above:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX >= 0x02030000

#else

/* These were introduced in Python 2.3: */
# define PY_LONG_LONG LONG_LONG

#endif

/*
  ----------------------------------------------------------------
  Python 2.2 and above:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX >= 0x02020000

# define HAVE_SUBCLASSABLE_TYPES 1

#else

/* These were introduced in Python 2.2: */
# define PyString_CheckExact PyString_Check
# define PyInt_CheckExact PyInt_Check
# define PyFloat_CheckExact PyFloat_Check
# define PyLong_CheckExact PyLong_Check

#endif

/*
  ----------------------------------------------------------------
  Python 1.6 and 2.0 only:
  ----------------------------------------------------------------
*/

/* Disabled: we don't support Python 1.6 and don't have a need for
   this API in any of the mx Tools (yet). */
#if 0
#if PY_VERSION_HEX >= 0x01060000 && PY_VERSION_HEX < 0x02010000

static PyObject *
PyObject_Unicode(PyObject *v)
{
	PyObject *res;
	
	if (v == NULL)
		res = PyString_FromString("<NULL>");
	else if (PyUnicode_Check(v)) {
		Py_INCREF(v);
		return v;
	}
	else if (PyString_Check(v))
	    	res = v;
	else if (v->ob_type->tp_str != NULL)
		res = (*v->ob_type->tp_str)(v);
	else {
		PyObject *func;
		if (!PyInstance_Check(v) ||
		    (func = PyObject_GetAttrString(v, "__str__")) == NULL) {
			PyErr_Clear();
			res = PyObject_Repr(v);
		}
		else {
		    	res = PyEval_CallObject(func, (PyObject *)NULL);
			Py_DECREF(func);
		}
	}
	if (res == NULL)
		return NULL;
	if (!PyUnicode_Check(res)) {
		PyObject* str;
		str = PyUnicode_FromObject(res);
		Py_DECREF(res);
		if (str)
			res = str;
		else
		    	return NULL;
	}
	return res;
}

#endif
#endif

/*
  ----------------------------------------------------------------
  Python 2.0 alpha + betas:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX >= 0x02000000 && PY_VERSION_HEX < 0x020000F0

# if defined(PyBuffer_Check) && !defined(HAVE_PYTHON_BUFFEROBJECTS)
#  define HAVE_PYTHON_BUFFEROBJECTS
# endif

#ifndef Py_PROTO
# define Py_PROTO(args) args
#endif

#endif

/*
  ----------------------------------------------------------------
  Python 1.6 and later:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX >= 0x01060000

#if defined(PyUnicode_Check) && !defined(HAVE_UNICODE)
# define HAVE_UNICODE
#endif

#endif

#if PY_VERSION_HEX >= 0x02020000

#if defined(HAVE_UNICODE) && !defined(Py_USING_UNICODE)
# undef HAVE_UNICODE
#endif

#endif

#ifndef HAVE_UNICODE
# undef PyUnicode_Check
# define PyUnicode_Check(obj) 0
#endif

/*
  ----------------------------------------------------------------
  Python < 1.6:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX < 0x01060000

#if !defined(PyObject_DEL)
# define PyObject_DEL(x) free(x)
# define PyObject_Del(x) free(x)
#endif

#endif

/*
  ----------------------------------------------------------------
  Python >= 1.5.2:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX >= 0x010502F0

# if defined(PyBuffer_Check) && !defined(HAVE_PYTHON_BUFFEROBJECTS)
#  define HAVE_PYTHON_BUFFEROBJECTS
# endif

#endif

/*
  ----------------------------------------------------------------
  Python >= 1.5.2 and prior to 2.0a1
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX >= 0x010502F0 && PY_VERSION_HEX < 0x02000000

/* Takes an arbitrary object which must support the (character, single
   segment) buffer interface and returns a pointer to a read-only
   memory location useable as character based input for subsequent
   processing.

   buffer and buffer_len are only set in case no error
   occurrs. Otherwise, -1 is returned and an exception set.

*/

static
int PyObject_AsCharBuffer(PyObject *obj,
			  const char **buffer,
			  int *buffer_len)
{
    PyBufferProcs *pb = obj->ob_type->tp_as_buffer;
    const char *pp;
    int len;

    if ( pb == NULL ||
	 pb->bf_getcharbuffer == NULL ||
	 pb->bf_getsegcount == NULL ) {
	PyErr_SetString(PyExc_TypeError,
			"expected a character buffer object");
	goto onError;
    }
    if ( (*pb->bf_getsegcount)(obj,NULL) != 1 ) {
	PyErr_SetString(PyExc_TypeError,
			"expected a single-segment buffer object");
	goto onError;
    }
    len = (*pb->bf_getcharbuffer)(obj,0,&pp);
    if (len < 0)
	goto onError;
    *buffer = pp;
    *buffer_len = len;
    return 0;

 onError:
    return -1;
}

/* Same as PyObject_AsCharBuffer() except that this API expects
   (readable, single segment) buffer interface and returns a pointer
   to a read-only memory location which can contain arbitrary data.

   buffer and buffer_len are only set in case no error
   occurrs. Otherwise, -1 is returned and an exception set.

*/

static
int PyObject_AsReadBuffer(PyObject *obj,
			  const void **buffer,
			  int *buffer_len)
{
    PyBufferProcs *pb = obj->ob_type->tp_as_buffer;
    void *pp;
    int len;

    if ( pb == NULL ||
	 pb->bf_getreadbuffer == NULL ||
	 pb->bf_getsegcount == NULL ) {
	PyErr_SetString(PyExc_TypeError,
			"expected a readable buffer object");
	goto onError;
    }
    if ( (*pb->bf_getsegcount)(obj,NULL) != 1 ) {
	PyErr_SetString(PyExc_TypeError,
			"expected a single-segment buffer object");
	goto onError;
    }
    len = (*pb->bf_getreadbuffer)(obj,0,&pp);
    if (len < 0)
	goto onError;
    *buffer = pp;
    *buffer_len = len;
    return 0;

 onError:
    return -1;
}

/* Takes an arbitrary object which must support the (writeable, single
   segment) buffer interface and returns a pointer to a writeable
   memory location in buffer of size buffer_len.

   buffer and buffer_len are only set in case no error
   occurrs. Otherwise, -1 is returned and an exception set.

*/

static
int PyObject_AsWriteBuffer(PyObject *obj,
			   void **buffer,
			   int *buffer_len)
{
    PyBufferProcs *pb = obj->ob_type->tp_as_buffer;
    void*pp;
    int len;

    if ( pb == NULL ||
	 pb->bf_getwritebuffer == NULL ||
	 pb->bf_getsegcount == NULL ) {
	PyErr_SetString(PyExc_TypeError,
			"expected a writeable buffer object");
	goto onError;
    }
    if ( (*pb->bf_getsegcount)(obj,NULL) != 1 ) {
	PyErr_SetString(PyExc_TypeError,
			"expected a single-segment buffer object");
	goto onError;
    }
    len = (*pb->bf_getwritebuffer)(obj,0,&pp);
    if (len < 0)
	goto onError;
    *buffer = pp;
    *buffer_len = len;
    return 0;

 onError:
    return -1;
}

#endif /* Python Version in [1.5.2b2, 2.0) */

/*
  ----------------------------------------------------------------
  Python 1.5.2b1 and older:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX <= 0x010502B1

/* These are missing from PC/python_nt.def and thus didn't get included
   in python1.5.lib on Windows platforms. */
#ifdef MS_WIN32
# define PyString_InternInPlace(x)
# define PyString_InternFromString(x) PyString_FromString(x)
#endif

#endif /* Python Version <= 1.5.2b1 */

/*
  ----------------------------------------------------------------
  Python 1.5.2a2 and older:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX <= 0x010502A2

extern long PyOS_strtol Py_PROTO((const char *, char **, int));
        
#endif /* Python Version <= 1.5.2a2 */

/*
  ----------------------------------------------------------------
  Python 1.5.2a1 and older:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX <= 0x010502A1

#ifndef PyList_SET_ITEM
# define PyList_SET_ITEM(l,i,w) PyList_GET_ITEM(l,i) = w
#endif

#endif /* Python Version < 1.5.2a1 */

/*
  ----------------------------------------------------------------
  Prior to Python 1.5:
  ----------------------------------------------------------------
*/

#if PY_VERSION_HEX < 0x010500F0

/* New in Python1.5: */
# undef  PyString_AS_STRING
# define PyString_AS_STRING(op) (((PyStringObject *)(op))->ob_sval)
# define PyString_GET_SIZE(op)  (((PyStringObject *)(op))->ob_size)
# define PyTuple_GET_SIZE(op)    (((PyTupleObject *)(op))->ob_size)
# define PyList_GET_SIZE(op)    (((PyListObject *)(op))->ob_size)

/* Changed since 1.4 */
# undef PyList_GET_ITEM
# define PyList_GET_ITEM(op, i) (((PyListObject *)(op))->ob_item[i])
# undef  PyInt_AS_LONG
# define PyInt_AS_LONG(op) (((PyIntObject *)(op))->ob_ival)
# undef PyFloat_AS_DOUBLE
# define PyFloat_AS_DOUBLE(op) (((PyFloatObject *)(op))->ob_fval)

/* This function is taken from error.c in Python 1.5...

   Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam, The
   Netherlands.

                        All Rights Reserved

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted, provided
   that the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation, and that the names of Stichting Mathematisch Centrum or
   CWI or Corporation for National Research Initiatives or CNRI not be used
   in advertising or publicity pertaining to distribution of the software
   without specific, written prior permission.

   While CWI is the initial source for this software, a modified version is
   made available by the Corporation for National Research Initiatives
   (CNRI) at the Internet address ftp://ftp.python.org.

   STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
   REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
   CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
   PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
   ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
   THIS SOFTWARE.

*/

#ifdef HAVE_STDARG_PROTOTYPES
PyObject *
PyErr_Format(PyObject *exception, const char *format, ...)
#else
PyObject *
PyErr_Format(exception, format, va_alist)
	PyObject *exception;
	const char *format;
	va_dcl
#endif
{
	va_list vargs;
	char buffer[500]; /* Caller is responsible for limiting the format */

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif

	vsprintf(buffer, format, vargs);
	PyErr_SetString(exception, buffer);
	return NULL;
}

/* Python 1.5 uses instances as exceptions, the 1.4 API only knows
   about strings. */
#define PyErr_NewException(name,base,dict) PyString_FromString(fullname);

/* Missing from rename2.h in Python 1.4 */
#ifndef PyVarObject
# define PyVarObject varobject
#endif

#endif /* Python Version < 1.5 */

#ifdef __cplusplus
}
#endif

/* EOF */
#endif
