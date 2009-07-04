#ifndef MXSTDLIB_H
#define MXSTDLIB_H

/* Standard stuff I use often -- not Python specific 

   Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
   Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com
   See the documentation for further copyright information or contact
   the author.

 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#ifndef INT_MAX
# define INT_MAX 2147483647
#endif
#ifndef LONG_MAX
# define LONG_MAX INT_MAX
#endif
#endif

/* --- My own macros for memory allocation... --------------------------- */

#ifdef MAL_MEM_DEBUG
# define newstruct(x) \
         (mxDebugPrintf("* malloc for struct "#x" (%s:%i)\n",__FILE__,__LINE__),\
	  (x *)malloc(sizeof(x)))
# define cnewstruct(x) \
         (mxDebugPrintf("* calloc for struct "#x" (%s:%i)\n",c,__FILE__,__LINE__),\
	  (x *)calloc(sizeof(x),1))
# define new(x,c) \
         (mxDebugPrintf("* malloc for "#c"=%i '"#x"'s (%s:%i)\n",c,__FILE__,__LINE__),\
	  (x *)malloc(sizeof(x)*(c)))
# define cnew(x,c) \
         (mxDebugPrintf("* calloc for "#c"=%i '"#x"'s (%s:%i)\n",c,__FILE__,__LINE__),\
	  (x *)calloc((c),sizeof(x)))
# define resize(var,x,c) \
         (mxDebugPrintf("* realloc array "#var" ("#x") at %X to size "#c"=%i (%s:%i)\n",var,c,__FILE__,__LINE__),\
	  (x *)realloc((void*)(var),sizeof(x)*(c)))
# define varresize(var,x,bytes) \
         (mxDebugPrintf("* realloc var "#var" ("#x") at %X to %i bytes (%s:%i)\n",var,bytes,__FILE__,__LINE__),\
	  (x *)realloc((void*)(var),(bytes)))
# define free(x) \
         (mxDebugPrintf("* freeing "#x" at %X (%s:%i)\n",x,__FILE__,__LINE__),\
	  free((void*)(x)))
#else
# define newstruct(x)		((x *)malloc(sizeof(x)))
# define cnewstruct(x)		((x *)calloc(sizeof(x),1))
# define new(x,c)		((x *)malloc(sizeof(x)*(c)))
# define cnew(x,c)		((x *)calloc((c),sizeof(x)))
# define resize(var,x,c)	((x *)realloc((void*)(var),sizeof(x)*(c)))
# define varresize(var,x,bytes)	((x *)realloc((void*)(var),(bytes)))
# define free(x) 		free((void*)(x))
#endif

/* --- Debugging output ------------------------------------------------- */

/* Use the flag MAL_DEBUG to enable debug processing.

   The flag MAL_DEBUG_WITH_PYTHON can be used to indicate that the
   object file will be linked with Python, so we can use Python APIs
   for the debug processing here.

*/
#ifdef MAL_DEBUG_WITH_PYTHON
# ifndef PYTHON_API_VERSION
#  error "mx.h must be included when compiling with MAL_DEBUG_WITH_PYTHON"
# endif
# ifndef MAL_DEBUG
#  define MAL_DEBUG
# endif
#else
# if defined(PYTHON_API_VERSION) && defined(MAL_DEBUG)
#  define MAL_DEBUG_WITH_PYTHON
# endif
#endif

/* Indicator for the availability of these interfaces: */

#define HAVE_MAL_DEBUG

/* Name of the environment variable defining the log file name
   to be used: */

#ifndef MAL_DEBUG_OUTPUTFILE_ENV_VARIABLE
# define MAL_DEBUG_OUTPUTFILE_ENV_VARIABLE "mxLogFile"
#endif

/* File name to be used for debug logging (each object file using this
   facility may set its own logging file) if no environment variable
   is set: */

#ifndef MAL_DEBUG_OUTPUTFILE
# define MAL_DEBUG_OUTPUTFILE "mx.log"
#endif

/* Name of the environment variable defining the log file prefix to be
   used (e.g. to direct all log files into a separate directory): */

#ifndef MAL_DEBUG_OUTPUTFILEPREFIX_ENV_VARIABLE
# define MAL_DEBUG_OUTPUTFILEPREFIX_ENV_VARIABLE "mxLogFileDir"
#endif

/* File name prefix to be used for log files, if no environment
   variable is set: */

#ifndef MAL_DEBUG_OUTPUTFILEPREFIX
# define MAL_DEBUG_OUTPUTFILEPREFIX ""
#endif

/* Log id to be used */

#ifndef MAL_DEBUG_LOGID
# define MAL_DEBUG_LOGID "New Log Session"
#endif

/* Debug printf() API

   Output is written to a log file or stream. If the output file is
   not yet open, the function will try to open the file as defined by
   the environment or the program defines.  The file remains open
   until the program terminates. Subsequent changes to the environment
   are not taken into account.

   The output file is deduced in the following way:

   1. get the filename from the environment, revert to the predefined
      value

   2. get the filename prefix from the environment, revert to
      the predefined value
   
   3. if filename is one of "stderr" or "stdout" use the native
      streams for output; otherwise try to open fileprefix + filename
      reverting to stderr in case this fails.

 */

static
int mxDebugPrintf(const char *format, ...)
{
    return 1;
}

#ifdef MAL_DEBUG

# ifdef MAL_DEBUG_WITH_PYTHON
/* Use the Python debug flag to enable debugging output (python -d) */
#  define DPRINTF if (Py_DebugFlag) mxDebugPrintf
#  define IF_DEBUGGING if (Py_DebugFlag) 
#  define DEBUGGING (Py_DebugFlag > 0)
# else

/* Always output debugging information */
#  define DPRINTF mxDebugPrintf
#  define IF_DEBUGGING  
#  define DEBUGGING (1)
# endif

#else

# ifndef _MSC_VER
/* This assumes that you are using an optimizing compiler which
   eliminates the resulting debug code. */
#  define DPRINTF if (0) mxDebugPrintf
#  define IF_DEBUGGING if (0) 
#  define DEBUGGING (0)
# else

/* MSVC doesn't do a good job here, so we use a different approach. */
#  define DPRINTF 0 && mxDebugPrintf
#  define IF_DEBUGGING if (0) 
#  define DEBUGGING (0)
# endif

#endif

/* --- Misc ------------------------------------------------------------- */

/* The usual bunch... */
#ifndef max
# define max(a,b) ((a>b)?(a):(b))
#endif
#ifndef min
# define min(a,b) ((a<b)?(a):(b))
#endif

/* Bit testing... returns 1 iff bit is on in value, 0 otherwise */
#ifndef testbit
# define testbit(value,bit) (((value) & (1<<(bit))) != 0)
#endif

/* Flag testing... returns 1 iff flag is on in value, 0 otherwise */
#ifndef testflag
# define testflag(value,flag) (((value) & (flag)) != 0)
#endif

/* EOF */
#endif

