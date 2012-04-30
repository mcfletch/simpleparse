#ifndef MXTEXTTOOLS_H
#define MXTEXTTOOLS_H
/* 
  mxTextTools -- Fast text manipulation routines

  Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com
*/

/* The extension's name; must be the same as the init function's suffix */
#define MXTEXTTOOLS_MODULE "mxTextTools"

#include "mxbmse.h"
#ifdef MXFASTSEARCH
# include "private/mxfse.h"
#endif

/* Include generic mx extension header file */
#include "mxh.h"

#ifdef MX_BUILDING_MXTEXTTOOLS
# define MXTEXTTOOLS_EXTERNALIZE MX_EXPORT
#else
# define MXTEXTTOOLS_EXTERNALIZE MX_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* --- Text Search Object ---------------------------------------*/

/* Algorithm values */
#define MXTEXTSEARCH_BOYERMOORE		0
#define MXTEXTSEARCH_FASTSEARCH		1
#define MXTEXTSEARCH_TRIVIAL		2

typedef struct {
    PyObject_HEAD
    PyObject *match; 		/* Match string object */
    PyObject *translate;	/* Translate string object or NULL */
    int algorithm;		/* Algorithm to be used */
    void *data; 		/* Internal data used by the algorithm or
				   NULL */
} mxTextSearchObject;

MXTEXTTOOLS_EXTERNALIZE(PyTypeObject) mxTextSearch_Type;

#define mxTextSearch_Check(v) \
        (((mxTextSearchObject *)(v))->ob_type == &mxTextSearch_Type)

/* Exporting these APIs for mxTextTools internal use only ! */

extern 
Py_ssize_t mxTextSearch_MatchLength(PyObject *self);

extern
Py_ssize_t mxTextSearch_SearchBuffer(PyObject *self,
			      char *text,
			      Py_ssize_t start,
			      Py_ssize_t stop,
			      Py_ssize_t *sliceleft,
			      Py_ssize_t *sliceright);

#ifdef HAVE_UNICODE
extern
Py_ssize_t mxTextSearch_SearchUnicode(PyObject *self,
			       Py_UNICODE *text,
			       Py_ssize_t start,
			       Py_ssize_t stop,
			       Py_ssize_t *sliceleft,
			       Py_ssize_t *sliceright);
#endif

/* --- Character Set Object -------------------------------------*/

/* Mode values */
#define MXCHARSET_8BITMODE	0
#define MXCHARSET_UCS2MODE	1
#define MXCHARSET_UCS4MODE	2

typedef struct {
    PyObject_HEAD
    PyObject *definition;       /* Character set definition */
    int mode;			/* Operation mode: 
				   0 - 8-bit character lookup 
				   1 - UCS-2 Unicode lookup
				   2 - UCS-4 Unicode lookup
				*/
    void *lookup;		/* Lookup table */
} mxCharSetObject;

MXTEXTTOOLS_EXTERNALIZE(PyTypeObject) mxCharSet_Type;

#define mxCharSet_Check(v) \
        (((mxCharSetObject *)(v))->ob_type == &mxCharSet_Type)


/* Exporting these APIs for mxTextTools internal use only ! */

extern
int mxCharSet_ContainsChar(PyObject *self,
			   register unsigned char ch);
    
#ifdef HAVE_UNICODE
extern
int mxCharSet_ContainsUnicodeChar(PyObject *self,
				  register Py_UNICODE ch);
#endif

extern
Py_ssize_t mxCharSet_Match(PyObject *self,
		    PyObject *text,
		    Py_ssize_t start,
		    Py_ssize_t stop,
		    int direction);

/* --- Tag Table Object -----------------------------------------*/

typedef struct {
    PyObject *tagobj;			/* Tag object to assign, call,
					   append, etc. or NULL */
    int cmd;				/* Command integer */
    int flags;				/* Command flags */
    PyObject *args;			/* Command arguments */
    int jne;				/* Non-match jump offset */
    int je;				/* Match jump offset */
} mxTagTableEntry;

#define MXTAGTABLE_STRINGTYPE	0
#define MXTAGTABLE_UNICODETYPE	1

typedef struct {
    PyObject_VAR_HEAD
    PyObject *definition;		/* Reference to the original
					   table definition or NULL;
					   needed for caching */
    int tabletype;			/* Type of compiled table:
					   0 - 8-bit string args
					   1 - Unicode args */
    mxTagTableEntry entry[1];		/* Variable length array of
					   mxTagTableEntry fields;
					   ob_size gives the number of
					   allocated entries. */
} mxTagTableObject;

MXTEXTTOOLS_EXTERNALIZE(PyTypeObject) mxTagTable_Type;

#define mxTagTable_Check(v) \
        (((mxTagTableObject *)(v))->ob_type == &mxTagTable_Type)

#define mxTagTable_Type(v) \
	(((mxTagTableObject *)(v))->tabletype)
#define mxTagTable_Definition(v) \
	(((mxTagTableObject *)(v))->definition)

/* Exporting these APIs for mxTextTools internal use only ! */
extern 
PyObject *mxTagTable_New(PyObject *definition,
			 int tabletype,
			 int cacheable);

/* --- Tagging Engine -------------------------------------------*/

/* Exporting these APIs for mxTextTools internal use only ! */

/* mxTextTools_TaggingEngine(): a table driven parser engine
   
   - return codes: rc = 2: match ok; rc = 1: match failed; rc = 0: error
   - doesn't check type of passed arguments !
   - doesn't increment reference counts of passed objects !
*/

extern 
int mxTextTools_TaggingEngine(PyObject *textobj,
			      Py_ssize_t text_start,	
			      Py_ssize_t text_stop,	
			      mxTagTableObject *table,
			      PyObject *taglist,
			      PyObject *context,
			      Py_ssize_t *next);

extern 
int mxTextTools_UnicodeTaggingEngine(PyObject *textobj,
				     Py_ssize_t text_start,	
				     Py_ssize_t text_stop,	
				     mxTagTableObject *table,
				     PyObject *taglist,
				     PyObject *context,
				     Py_ssize_t *next);

/* Command integers for cmd; see Constants/TagTable.py for details */

/* Low-level string matching, using the same simple logic:
   - match has to be a string 
   - they only modify x (the current position in text)
*/
#define MATCH_ALLIN 		11
#define MATCH_ALLNOTIN 		12
#define MATCH_IS 		13
#define MATCH_ISIN 		14
#define MATCH_ISNOTIN 		15

#define MATCH_WORD 		21
#define MATCH_WORDSTART       	22
#define MATCH_WORDEND		23

#define MATCH_ALLINSET 		31
#define MATCH_ISINSET		32

#define MATCH_ALLINCHARSET	41
#define MATCH_ISINCHARSET	42

#define MATCH_MAX_LOWLEVEL	99

/* Jumps and other low-level special commands */

#define MATCH_FAIL		100
#define MATCH_JUMP 		MATCH_FAIL

#define MATCH_EOF 		101
#define MATCH_SKIP 		102
#define MATCH_MOVE		103

#define MATCH_JUMPTARGET	104

#define MATCH_MAX_SPECIALS	199

/* Higher-level string matching */

#define MATCH_SWORDSTART	211
#define MATCH_SWORDEND		212
#define MATCH_SFINDWORD		213
#define MATCH_NOWORD		MATCH_SWORDSTART

/* Higher-level special commands */
#define MATCH_CALL 		201
#define MATCH_CALLARG 		202
#define MATCH_TABLE 		203
#define MATCH_SUBTABLE 		207
#define MATCH_TABLEINLIST 	204
#define MATCH_SUBTABLEINLIST 	208
#define MATCH_LOOP 		205
#define MATCH_LOOPCONTROL	206

/* Special argument integers */
#define MATCH_JUMP_TO		0
#define MATCH_JUMP_MATCHOK	1000000
#define MATCH_JUMP_MATCHFAIL	-1000000
#define MATCH_MOVE_EOF		-1
#define MATCH_MOVE_BOF		0
#define MATCH_FAIL_HERE		1
#define MATCH_THISTABLE		999
#define MATCH_LOOPCONTROL_BREAK	0
#define MATCH_LOOPCONTROL_RESET -1

/* Flags set in cmd (>=256) */
#define MATCH_CALLTAG		(1 << 8)
#define MATCH_APPENDTAG 	(1 << 9)
#define MATCH_APPENDTAGOBJ	(1 << 10)
#define MATCH_APPENDMATCH	(1 << 11)
#define MATCH_LOOKAHEAD		(1 << 12)

/* EOF */
#ifdef __cplusplus
}
#endif
#endif
