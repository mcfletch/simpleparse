#ifndef MXPYAPI_H
#define MXPYAPI_H

/* mxpyapi.h

   This header file includes some new APIs that are not available in
   older API versions, yet are used by the mx-Extensions.

   Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
   Copyright (c) 2000-2003, eGenix.com Software GmbH; mailto:info@egenix.com

*/

#if defined(PyUnicode_Check) && !defined(HAVE_UNICODE)
# define HAVE_UNICODE
#endif

#if defined(HAVE_UNICODE) && !defined(Py_USING_UNICODE)
# undef HAVE_UNICODE
#endif

#ifndef HAVE_UNICODE
# undef PyUnicode_Check
# define PyUnicode_Check(obj) 0
#endif

#if PY_MAJOR_VERSION >= 3
#define PyInt_FromLong                  PyLong_FromLong
#define PyInt_Check                     PyLong_Check
#define PyInt_AS_LONG                   PyLong_AS_LONG

#define PyString_FromStringAndSize      PyBytes_FromStringAndSize
#define PyString_AsString               PyBytes_AsString
#define PyString_FromString             PyBytes_FromString
#define PyString_Check                  PyBytes_Check
#define PyString_FromFormat             PyBytes_FromFormat
#define PyString_GET_SIZE               PyBytes_GET_SIZE
#define PyString_AS_STRING              PyBytes_AS_STRING
#define _PyString_Resize                _PyBytes_Resize
#endif

/* EOF */
#endif
