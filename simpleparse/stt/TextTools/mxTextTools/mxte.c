/* 
  mxte -- A table driven tagging engine for Python (Version 0.9)

  Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com
*/

/* Debugging switches */
/*#define MAL_DEBUG*/
/*#define MAL_REF_DEBUG*/

/* Logging file used by debugging facility */
#ifndef MAL_DEBUG_OUTPUTFILE
# define MAL_DEBUG_OUTPUTFILE "mxTagEngine.log"
#endif

#include "mx.h"
#include "mxstdlib.h"
#include "mxTextTools.h"



/* --- Tagging Engine --- 8-bit String version ---------------------------- */

#undef TE_STRING_CHECK 
#define TE_STRING_CHECK(obj) PyString_Check(obj)
#undef TE_STRING_AS_STRING
#define TE_STRING_AS_STRING(obj) PyString_AS_STRING(obj)
#undef TE_STRING_GET_SIZE
#define TE_STRING_GET_SIZE(obj) PyString_GET_SIZE(obj)
#undef TE_STRING_FROM_STRING
#define TE_STRING_FROM_STRING(str, size) PyString_FromStringAndSize(str, size)
#undef TE_CHAR
#define TE_CHAR char
#undef TE_HANDLE_MATCH
#define TE_HANDLE_MATCH string_handle_match
#undef TE_ENGINE_API
#define TE_ENGINE_API mxTextTools_TaggingEngine
#undef TE_TABLETYPE
#define TE_TABLETYPE MXTAGTABLE_STRINGTYPE
#undef TE_SEARCHAPI
#define TE_SEARCHAPI mxTextSearch_SearchBuffer

#include "mxte_impl.h"

/* --- Old Unicode Engine DISABLED - using modern engines only --- */
/* All Unicode processing now goes through the modern tri-engine system */
