/* Marc-Andre's Hex version determination code */
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


#if 0
/* PY_HEX_VERSION < 0x02020000 */
/*  Python 2.2 features backported to earlier Python versions */


#ifndef PYSTRING_FROMFORMAT_BACKPORT
#define PYSTRING_FROMFORMAT_BACKPORT
/* PyString_FromFormat back-porting code

	There are no docs for when PyString_FromFormat shows up that I can see,
	appears to be Python version 2.2.0

	This PyString_FromFormat back-porting code is from Python 2.2.1:
		Copyright (c) 2001, 2002 Python Software Foundation.
		All Rights Reserved.

		Copyright (c) 2000 BeOpen.com.
		All Rights Reserved.

		Copyright (c) 1995-2001 Corporation for National Research Initiatives.
		All Rights Reserved.

		Copyright (c) 1991-1995 Stichting Mathematisch Centrum, Amsterdam.
		All Rights Reserved.

*/


#include <ctype.h>
PyObject *
PyString_FromFormatV(const char *format, va_list vargs)
{
	va_list count;
	int n = 0;
	const char* f;
	char *s;
	PyObject* string;

#ifdef VA_LIST_IS_ARRAY
	memcpy(count, vargs, sizeof(va_list));
#else
	count = vargs;
#endif
	/* step 1: figure out how large a buffer we need */
	for (f = format; *f; f++) {
		if (*f == '%') {
			const char* p = f;
			while (*++f && *f != '%' && !isalpha(Py_CHARMASK(*f)))
				;

			/* skip the 'l' in %ld, since it doesn't change the
			   width.  although only %d is supported (see
			   "expand" section below), others can be easily
			   added */
			if (*f == 'l' && *(f+1) == 'd')
				++f;
			
			switch (*f) {
			case 'c':
				(void)va_arg(count, int);
				/* fall through... */
			case '%':
				n++;
				break;
			case 'd': case 'i': case 'x':
				(void) va_arg(count, int);
				/* 20 bytes is enough to hold a 64-bit
				   integer.  Decimal takes the most space.
				   This isn't enough for octal. */
				n += 20;
				break;
			case 's':
				s = va_arg(count, char*);
				n += strlen(s);
				break;
			case 'p':
				(void) va_arg(count, int);
				/* maximum 64-bit pointer representation:
				 * 0xffffffffffffffff
				 * so 19 characters is enough.
				 * XXX I count 18 -- what's the extra for?
				 */
				n += 19;
				break;
			default:
				/* if we stumble upon an unknown
				   formatting code, copy the rest of
				   the format string to the output
				   string. (we cannot just skip the
				   code, since there's no way to know
				   what's in the argument list) */ 
				n += strlen(p);
				goto expand;
			}
		} else
			n++;
	}
 expand:
	/* step 2: fill the buffer */
	/* Since we've analyzed how much space we need for the worst case,
	   use sprintf directly instead of the slower PyOS_snprintf. */
	string = PyString_FromStringAndSize(NULL, n);
	if (!string)
		return NULL;
	
	s = PyString_AsString(string);

	for (f = format; *f; f++) {
		if (*f == '%') {
			const char* p = f++;
			int i, longflag = 0;
			/* parse the width.precision part (we're only
			   interested in the precision value, if any) */
			n = 0;
			while (isdigit(Py_CHARMASK(*f)))
				n = (n*10) + *f++ - '0';
			if (*f == '.') {
				f++;
				n = 0;
				while (isdigit(Py_CHARMASK(*f)))
					n = (n*10) + *f++ - '0';
			}
			while (*f && *f != '%' && !isalpha(Py_CHARMASK(*f)))
				f++;
			/* handle the long flag, but only for %ld.  others
			   can be added when necessary. */
			if (*f == 'l' && *(f+1) == 'd') {
				longflag = 1;
				++f;
			}

			switch (*f) {
			case 'c':
				*s++ = va_arg(vargs, int);
				break;
			case 'd':
				if (longflag)
					sprintf(s, "%ld", va_arg(vargs, long));
				else
					sprintf(s, "%d", va_arg(vargs, int));
				s += strlen(s);
				break;
			case 'i':
				sprintf(s, "%i", va_arg(vargs, int));
				s += strlen(s);
				break;
			case 'x':
				sprintf(s, "%x", va_arg(vargs, int));
				s += strlen(s);
				break;
			case 's':
				p = va_arg(vargs, char*);
				i = strlen(p);
				if (n > 0 && i > n)
					i = n;
				memcpy(s, p, i);
				s += i;
				break;
			case 'p':
				sprintf(s, "%p", va_arg(vargs, void*));
				/* %p is ill-defined:  ensure leading 0x. */
				if (s[1] == 'X')
					s[1] = 'x';
				else if (s[1] != 'x') {
					memmove(s+2, s, strlen(s)+1);
					s[0] = '0';
					s[1] = 'x';
				}
				s += strlen(s);
				break;
			case '%':
				*s++ = '%';
				break;
			default:
				strcpy(s, p);
				s += strlen(s);
				goto end;
			}
		} else
			*s++ = *f;
	}
	
 end:
	_PyString_Resize(&string, s - PyString_AS_STRING(string));
	return string;
}
	
PyObject *
PyString_FromFormat(const char *format, ...) 
{
	PyObject* ret;
	va_list vargs;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif
	ret = PyString_FromFormatV(format, vargs);
	va_end(vargs);
	return ret;
}
/* end PyString_FromFormat back-porting code */
#endif /* PYSTRING_FROMFORMAT_BACKPORT */

#endif /* < Python 2.2 */

