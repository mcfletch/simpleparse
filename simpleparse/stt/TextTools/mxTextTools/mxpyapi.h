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

/* EOF */
#endif
