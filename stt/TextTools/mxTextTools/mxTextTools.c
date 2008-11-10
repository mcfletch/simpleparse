/* 
  mxTextTools -- Fast text manipulation routines

  Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com
*/

/* We want all our symbols to be exported */
#define MX_BUILDING_MXTEXTTOOLS

/* Logging file used by debugging facility */
#ifndef MAL_DEBUG_OUTPUTFILE
# define MAL_DEBUG_OUTPUTFILE "mxTextTools.log"
#endif

#include "mx.h"
#include "mxTextTools.h"
#include <ctype.h>

#define VERSION "2.1.0"

/* Initial list size used by e.g. setsplit(), setsplitx(),... */
#define INITIAL_LIST_SIZE 64

/* Maximum TagTable cache size. If this limit is reached, the cache
   is cleared to make room for new compile TagTables. */
#define MAX_TAGTABLES_CACHE_SIZE 100

/* Define this to enable the copy-protocol (__copy__, __deepcopy__) */
#define COPY_PROTOCOL

/* --- module doc-string -------------------------------------------------- */

static char *Module_docstring = 

 MXTEXTTOOLS_MODULE" -- Tools for fast text processing. Version "VERSION"\n\n"

 "Copyright (c) 1997-2000, Marc-Andre Lemburg; mailto:mal@lemburg.com\n"
 "Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com\n\n"
 "Copyright (c) 2003-2006, Mike Fletcher; mailto:mcfletch@vrplumber.com\n\n"

 "                 All Rights Reserved\n\n"
 "See the documentation for further information on copyrights,\n"
 "or contact the author."
;

/* --- internal macros ---------------------------------------------------- */

/* --- module globals ----------------------------------------------------- */

/* Translation strings for the 8-bit versions of lower() and upper() */
static PyObject *mx_ToUpper;
static PyObject *mx_ToLower;

static PyObject *mxTextTools_Error;	/* mxTextTools specific error */

static PyObject *mxTextTools_TagTables;	/* TagTable cache dictionary */

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
    return PyString_FromStringAndSize(tr,sizeof(tr));
}

static
PyObject *mxTextTools_ToLower(void)
{
    char tr[256];
    Py_ssize_t i;
    
    for (i = 0; i < 256; i++)
	tr[i] = tolower((char)i);
    return PyString_FromStringAndSize(tr,sizeof(tr));
}

/* Create an exception object, insert it into the module dictionary
   under the given name and return the object pointer; this is NULL in
   case an error occurred. base can be given to indicate the base
   object to be used by the exception object. It should be NULL
   otherwise */

static 
PyObject *insexc(PyObject *moddict,
		 char *name,
		 PyObject *base)
{
    PyObject *v;
    char fullname[256];
    char *modname;
    char *dot;
    
    v = PyDict_GetItemString(moddict, "__name__");
    if (v == NULL)
	modname = NULL;
    else
	modname = PyString_AsString(v);
    if (modname == NULL) {
	PyErr_Clear();
	modname = MXTEXTTOOLS_MODULE;
    }
    /* The symbols from this extension are imported into
       simpleparse.stt.TextTools. We trim the name to not confuse the user with an
       overly long package path. */
    strcpy(fullname, modname);
    dot = strchr(fullname, '.');
    if (dot)
	dot = strchr(dot+1, '.');
    if (dot)
	strcpy(dot+1, name);
    else
	sprintf(fullname, "%s.%s", modname, name);

    v = PyErr_NewException(fullname, base, NULL);
    if (v == NULL)
	return NULL;
    if (PyDict_SetItemString(moddict,name,v))
	return NULL;
    return v;
}

/* Helper for adding integer constants to a dictionary. Check for
   errors with PyErr_Occurred() */
static 
void insint(PyObject *dict,
	    char *name,
	    int value)
{
    PyObject *v = PyInt_FromLong((long)value);
    PyDict_SetItemString(dict, name, v);
    Py_XDECREF(v);
}

/* --- module interface --------------------------------------------------- */

/* --- Text Search Object ----------------------------------------------*/

staticforward PyMethodDef mxTextSearch_Methods[];

/* allocation */

static
PyObject *mxTextSearch_New(PyObject *match,
			   PyObject *translate,
			   int algorithm)
{
    mxTextSearchObject *so;

    so = PyObject_NEW(mxTextSearchObject, &mxTextSearch_Type);
    if (so == NULL) 
	return NULL;
    so->data = NULL;
    so->translate = NULL;
    so->match = NULL;

    Py_INCREF(match);
    so->match = match;
    
    if (translate == Py_None)
	translate = NULL;
    else if (translate) {
	Py_Assert(PyString_Check(translate),
		  PyExc_TypeError,
		  "translate table must be a string");
	Py_Assert(PyString_GET_SIZE(translate) == 256,
		  PyExc_TypeError,
		  "translate string must have exactly 256 chars");
	Py_INCREF(translate);
    }
    so->translate = translate;

    /* Init algorithm */
    so->algorithm = algorithm;
    switch (algorithm) {

    case MXTEXTSEARCH_BOYERMOORE:
	Py_Assert(PyString_Check(match),
		  PyExc_TypeError,
		  "match must be a string for Boyer-Moore");
	so->data = bm_init(PyString_AS_STRING(match),
			   PyString_GET_SIZE(match));
	Py_Assert(so->data != NULL,
		  PyExc_TypeError,
		  "error initializing the search object");
	break;

#ifdef MXFASTSEARCH	
    case MXTEXTSEARCH_FASTSEARCH:
	Py_Assert(PyString_Check(match),
		  PyExc_TypeError,
		  "match must be a string for FastSearch");
	so->data = fs_init(PyString_AS_STRING(match),
			   PyString_GET_SIZE(match));
	Py_Assert(so->data != NULL,
		  PyExc_TypeError,
		  "error initializing the search object");
	break;
#endif	

    case MXTEXTSEARCH_TRIVIAL:
	Py_Assert(PyString_Check(match) || PyUnicode_Check(match),
		  PyExc_TypeError,
		  "match must be a string or unicode");
	Py_Assert(so->translate == NULL,
		  PyExc_TypeError,
		  "trivial search algorithm does not support translate");
	break;

    default:
	Py_Error(PyExc_ValueError,
		 "unknown or unsupported algorithm");

    }
    return (PyObject *)so;

 onError:
    Py_DECREF(so);
    return NULL;
}

Py_C_Function_WithKeywords(
                mxTextSearch_TextSearch,
	       "TextSearch(match[,translate=None,algorithm=default_algorithm])\n\n"
	       "Create a substring search object for the string match;\n"
	       "translate is an optional translate-string like the one used\n"
	       "in the module re."
		)
{
    PyObject *match = 0;
    PyObject *translate = 0;
    int algorithm = -424242;

    Py_KeywordsGet3Args("O|Oi:TextSearch",match,translate,algorithm);

    if (algorithm == -424242) {
	if (PyUnicode_Check(match))
	    algorithm = MXTEXTSEARCH_TRIVIAL;
	else
#ifdef MXFASTSEARCH
	    algorithm = MXTEXTSEARCH_BOYERMOORE;
#else
	    algorithm = MXTEXTSEARCH_BOYERMOORE;
#endif
    }
    return mxTextSearch_New(match, translate, algorithm);

 onError:
    return NULL;
}

static 
void mxTextSearch_Free(mxTextSearchObject *so)
{
    if (so->data) {
	switch  (so->algorithm) {

	case MXTEXTSEARCH_BOYERMOORE:
	    bm_free(so->data);
	    break;

#ifdef MXFASTSEARCH	
	case MXTEXTSEARCH_FASTSEARCH:
	    fs_free(so->data);
	    break;
#endif
	case MXTEXTSEARCH_TRIVIAL:
	    break;
	    
	}
    }
    Py_XDECREF(so->match);
    Py_XDECREF(so->translate);
    PyObject_Del(so);
}

/* C APIs */

#define so ((mxTextSearchObject *)self)

/* Get the match length from an TextSearch object or -1 in case of an
   error. */

Py_ssize_t mxTextSearch_MatchLength(PyObject *self)
{
    Py_Assert(mxTextSearch_Check(self),
	      PyExc_TypeError,
	      "expected a TextSearch object");

    switch  (so->algorithm) {

    case MXTEXTSEARCH_BOYERMOORE:
	return BM_MATCH_LEN(so->data);
	break;

#ifdef MXFASTSEARCH	
    case MXTEXTSEARCH_FASTSEARCH:
	return FS_MATCH_LEN(so->data);
	break;
#endif		

    case MXTEXTSEARCH_TRIVIAL:
	if (PyString_Check(so->match))
	    return PyString_GET_SIZE(so->match);
#ifdef HAVE_UNICODE
	else if (PyUnicode_Check(so->match))
	    return PyUnicode_GET_SIZE(so->match);
#endif
	break;

    }

    Py_Error(mxTextTools_Error,
	     "internal error");

 onError:
    return -1;
}

static
Py_ssize_t trivial_search(const char *text,
		   Py_ssize_t start,
		   Py_ssize_t stop,
		   const char *match,
		   Py_ssize_t match_len)
{
    Py_ssize_t ml1 = match_len - 1;
    register const char *tx = &text[start];
    register Py_ssize_t x = start;

    if (ml1 < 0) 
	return start;

    /* Brute-force method; from right to left */
    for (;;) {
	register Py_ssize_t j = ml1;
	register const char *mj = &match[j];

	if (x + j >= stop)
	    /* reached eof: no match */
	    return start;

	/* scan from right to left */
	for (tx += j; j >= 0 && *tx == *mj; 
	     tx--, mj--, j--) ;

	if (j < 0) {
	    /* found */
	    x += ml1 + 1;
	    return x;
	}
	/* not found: rewind and advance one char */
	tx -= j - 1;
	x++;
    }
    return start;
}

#ifdef HAVE_UNICODE
static
Py_ssize_t trivial_unicode_search(const Py_UNICODE *text,
			   Py_ssize_t start,
			   Py_ssize_t stop,
			   const Py_UNICODE *match,
			   Py_ssize_t match_len)
{
    Py_ssize_t ml1 = match_len - 1;
    register const Py_UNICODE *tx = &text[start];
    register Py_ssize_t x = start;

    if (ml1 < 0) 
	return start;

    /* Brute-force method; from right to left */
    for (;;) {
	register Py_ssize_t j = ml1;
	register const Py_UNICODE *mj = &match[j];

	if (x + j >= stop)
	    /* reached eof: no match */
	    return start;

	/* scan from right to left */
	for (tx += j; j >= 0 && *tx == *mj; 
	     tx--, mj--, j--) ;

	if (j < 0) {
	    /* found */
	    x += ml1 + 1;
	    return x;
	}
	/* not found: rewind and advance one char */
	tx -= j - 1;
	x++;
    }
    return start;
}
#endif

/* Search for the match in text[start:stop]. 

   Returns 1 in case a match was found and sets sliceleft, sliceright
   to the matching slice.

   Returns 0 in case no match was found and -1 in case of an error.

*/

Py_ssize_t mxTextSearch_SearchBuffer(PyObject *self,
			      char *text,
			      Py_ssize_t start,
			      Py_ssize_t stop,
			      Py_ssize_t *sliceleft,
			      Py_ssize_t *sliceright)
{
    Py_ssize_t nextpos;
    Py_ssize_t match_len;

    Py_Assert(mxTextSearch_Check(self),
	      PyExc_TypeError,
	      "expected a TextSearch object");

    switch  (so->algorithm) {

    case MXTEXTSEARCH_BOYERMOORE:
	if (so->translate) {
	    /* search with translate table */
	    nextpos = bm_tr_search((mxbmse_data *)so->data,
				   text,
				   start,
				   stop,
				   PyString_AS_STRING(so->translate));
	}
	else {
	    /* exact search */
	    nextpos = bm_search((mxbmse_data *)so->data,
				text,
				start,
				stop);
	}
	match_len = BM_MATCH_LEN(so->data);
	break;

#ifdef MXFASTSEARCH
    case MXTEXTSEARCH_FASTSEARCH:
	if (so->translate) {
	    /* search with translate table */
	    nextpos = fs_tr_search((mxfse_data *)so->data,
				   text,
				   start,
				   stop,
				   PyString_AS_STRING(so->translate));
	}
	else {
	    /* exact search */
	    nextpos = fs_search((mxfse_data *)so->data,
				text,
				start,
				stop);
	}
	match_len = FS_MATCH_LEN(so->data);
	break;
#endif
	
    case MXTEXTSEARCH_TRIVIAL:
	{
	    const char *match;

	    if (PyString_Check(so->match)) {
		match = PyString_AS_STRING(so->match);
		match_len = PyString_GET_SIZE(so->match);
	    }
	    else if (PyObject_AsCharBuffer(so->match, &match, &match_len))
		goto onError;
	    nextpos = trivial_search(text,
				     start,
				     stop,
				     match,
				     match_len);
	}
	break;

    default:
	Py_Error(mxTextTools_Error,
		 "unknown algorithm type in mxTextSearch_SearchBuffer");

    }
    /* Found ? */
    if (nextpos != start) {
	if (sliceleft)
	    *sliceleft = nextpos - match_len;
	if (sliceright)
	    *sliceright = nextpos;
	return 1;
    }
    /* Not found */
    return 0;

 onError:
    return -1;
}

#ifdef HAVE_UNICODE
Py_ssize_t mxTextSearch_SearchUnicode(PyObject *self,
			       Py_UNICODE *text,
			       Py_ssize_t start,
			       Py_ssize_t stop,
			       Py_ssize_t *sliceleft,
			       Py_ssize_t *sliceright)
{
    Py_ssize_t nextpos;
    Py_ssize_t match_len;

    Py_Assert(mxTextSearch_Check(self),
	      PyExc_TypeError,
	      "expected a TextSearch object");

    switch  (so->algorithm) {

    case MXTEXTSEARCH_BOYERMOORE:
	Py_Error(PyExc_TypeError,
		 "Boyer-Moore search algorithm does not support Unicode");
	break;

#ifdef MXFASTSEARCH
    case MXTEXTSEARCH_FASTSEARCH:
	Py_Error(PyExc_TypeError,
		 "FastSearch search algorithm does not support Unicode");
#endif
	
    case MXTEXTSEARCH_TRIVIAL:
	{
	    PyObject *u;
	    Py_UNICODE *match;

	    if (PyUnicode_Check(so->match)) {
		u = NULL;
		match = PyUnicode_AS_UNICODE(so->match);
		match_len = PyUnicode_GET_SIZE(so->match);
	    }
	    else {
		u = PyUnicode_FromEncodedObject(so->match, NULL, NULL);
		if (u == NULL)
		    goto onError;
		match = PyUnicode_AS_UNICODE(u);
		match_len = PyUnicode_GET_SIZE(u);
	    }
	    nextpos = trivial_unicode_search(text,
					     start,
					     stop,
					     match,
					     match_len);
	    Py_XDECREF(u);
	}
	break;

    default:
	Py_Error(mxTextTools_Error,
		 "unknown algorithm type in mxTextSearch_SearchUnicode");

    }
    /* Found ? */
    if (nextpos != start) {
	if (sliceleft)
	    *sliceleft = nextpos - match_len;
	if (sliceright)
	    *sliceright = nextpos;
	return 1;
    }
    /* Not found */
    return 0;

 onError:
    return -1;
}
#endif

/* methods */

Py_C_Function( mxTextSearch_search,
	       "TextSearch.search(text,start=0,stop=len(text))\n\n"
	       "Search for the substring in text, looking only at the\n"
	       "slice [start:stop] and return the slice (l,r)\n"
	       "where the substring was found, (start,start) otherwise.")
{
    PyObject *text;
    Py_ssize_t start = 0;
    Py_ssize_t stop = INT_MAX;
    Py_ssize_t sliceleft, sliceright;
    int rc;

    Py_Get3Args("O|ii:TextSearch.search",
		text,start,stop);

    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, stop);
	rc = mxTextSearch_SearchBuffer(self,
				       PyString_AS_STRING(text),
				       start, 
				       stop, 
				       &sliceleft, 
				       &sliceright);
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, start, stop);
	rc = mxTextSearch_SearchUnicode(self,
					PyUnicode_AS_UNICODE(text),
					start, 
					stop, 
					&sliceleft, 
					&sliceright);
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    if (rc < 0)
	goto onError;
    if (rc == 0) {
	sliceleft = start;
	sliceright = start;
    }

    /* Return the slice */
    Py_Return2("ii", sliceleft, sliceright);

 onError:
    return NULL;
}

Py_C_Function( mxTextSearch_find,
	       "TextSearch.find(text,start=0,stop=len(text))\n\n"
	       "Search for the substring in text, looking only at the\n"
	       "slice [start:stop] and return the index\n"
	       "where the substring was found, -1 otherwise.")
{
    PyObject *text;
    Py_ssize_t start = 0;
    Py_ssize_t stop = INT_MAX;
    Py_ssize_t sliceleft, sliceright;
    int rc;

    Py_Get3Args("O|ii:TextSearch.find",
		text,start,stop);

    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, stop);
	rc = mxTextSearch_SearchBuffer(self,
				       PyString_AS_STRING(text),
				       start, 
				       stop, 
				       &sliceleft, 
				       &sliceright);
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, start, stop);
	rc = mxTextSearch_SearchUnicode(self,
					PyUnicode_AS_UNICODE(text),
					start, 
					stop, 
					&sliceleft, 
					&sliceright);
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    if (rc < 0)
	goto onError;
    if (rc == 0)
	sliceleft = -1;
    return PyInt_FromLong(sliceleft);

 onError:
    return NULL;
}

Py_C_Function( mxTextSearch_findall,
	       "TextSearch.findall(text,start=0,stop=len(text))\n\n"
	       "Search for the substring in text, looking only at the\n"
	       "slice [start:stop] and return a list of all\n"
	       "non overlapping slices (l,r) in text where the match\n"
	       "string can be found.")
{
    PyObject *text;
    PyObject *list = 0;
    Py_ssize_t start = 0;
    Py_ssize_t stop = INT_MAX;
    Py_ssize_t stop_index;
    Py_ssize_t match_len;
    Py_ssize_t listsize = INITIAL_LIST_SIZE;
    Py_ssize_t listitem = 0;

    Py_Get3Args("O|ii:TextSearch.findall",
		text,start,stop);

    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, stop);
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, start, stop);
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    
    list = PyList_New(listsize);
    if (!list)
	goto onError;

    match_len = mxTextSearch_MatchLength(self);
    if (match_len < 0)
	goto onError;
    stop_index = stop - match_len;

    while (start <= stop_index) {
	register PyObject *t,*v;
	int rc;
	Py_ssize_t sliceleft, sliceright;

	/* exact search */
	if (PyString_Check(text))
	    rc = mxTextSearch_SearchBuffer(self,
					   PyString_AS_STRING(text),
					   start, 
					   stop, 
					   &sliceleft, 
					   &sliceright);
#ifdef HAVE_UNICODE
	else if (PyUnicode_Check(text))
	    rc = mxTextSearch_SearchUnicode(self,
					    PyUnicode_AS_UNICODE(text),
					    start, 
					    stop, 
					    &sliceleft, 
					    &sliceright);
#endif
	else
	    break;
	if (rc < 0)
	    goto onError;
	if (rc == 0)
	    break;

	/* Build slice and append to list */
	t = PyTuple_New(2);
	if (!t) 
	    goto onError;
	v = PyInt_FromLong(sliceleft);
	if (!v)
	    goto onError;
	PyTuple_SET_ITEM(t,0,v);
	v = PyInt_FromLong(sliceright);
	if (!v)
	    goto onError;
	PyTuple_SET_ITEM(t,1,v);

	if (listitem < listsize)
	    PyList_SET_ITEM(list, listitem, t);
	else {
	    PyList_Append(list, t);
	    Py_DECREF(t);
	}
	listitem++;

	start = sliceright;
    }

    /* Resize list if necessary */
    if (listitem < listsize)
	PyList_SetSlice(list, listitem, listsize, (PyObject*)NULL);

    return list;

 onError:
    Py_XDECREF(list);
    return NULL;
}

#ifdef COPY_PROTOCOL
Py_C_Function( mxTextSearch_copy,
	       "copy([memo])\n\n"
	       "Return a new reference for the instance. This function\n"
	       "is used for the copy-protocol. Real copying doesn't take\n"
	       "place, since the instances are immutable.")
{
    PyObject *memo;
    
    Py_GetArg("|O",memo);
    Py_INCREF(so);
    return (PyObject *)so;
 onError:
    return NULL;
}
#endif

#undef so

/* --- slots --- */

static 
PyObject *mxTextSearch_Repr(mxTextSearchObject *self)
{
    char *algoname;
    PyObject *v;
    char t[500], *reprstr;

    v = PyObject_Repr(self->match);
    if (v == NULL)
	return NULL;
    reprstr = PyString_AsString(v);
    if (reprstr == NULL)
	return NULL;

    switch (self->algorithm) {
    case MXTEXTSEARCH_BOYERMOORE:
	algoname = "Boyer-Moore";
	break;
#ifdef MXFASTSEARCH	
    case MXTEXTSEARCH_FASTSEARCH:
	algoname = "FastSearch";
	break;
#endif		
    case MXTEXTSEARCH_TRIVIAL:
	algoname = "Trivial";
	break;
    default:
	algoname = "";
    }

    sprintf(t, "<%.50s TextSearch object for %.400s at 0x%lx>",
	    algoname, reprstr, (long)self);
    Py_DECREF(v);
    return PyString_FromString(t);
}

static 
PyObject *mxTextSearch_GetAttr(mxTextSearchObject *self,
			char *name)
{
    PyObject *v;
    
    if (Py_WantAttr(name,"match")) {
	v = self->match;
	Py_INCREF(v);
	return v;
    }
    else if (Py_WantAttr(name,"translate")) {
        v = self->translate;
	if (v == NULL)
	    v = Py_None;
	Py_INCREF(v);
	return v;
    }
    else if (Py_WantAttr(name,"algorithm"))
        return PyInt_FromLong(self->algorithm);
    else if (Py_WantAttr(name,"__members__"))
	return Py_BuildValue("[sss]",
			     "match", "translate", "algorithm");
    
    return Py_FindMethod(mxTextSearch_Methods, (PyObject *)self, (char *)name);
}

/* Python Type Table */

PyTypeObject mxTextSearch_Type = {
        PyObject_HEAD_INIT(0)		/* init at startup ! */
	0,			  	/*ob_size*/
	"TextSearch",		  	/*tp_name*/
	sizeof(mxTextSearchObject),	/*tp_basicsize*/
	0,			  	/*tp_itemsize*/
	/* methods */
	(destructor)mxTextSearch_Free,	/*tp_dealloc*/
	(printfunc)0,			/*tp_print*/
	(getattrfunc)mxTextSearch_GetAttr,  	/*tp_getattr*/
	(setattrfunc)0,		  	/*tp_setattr*/
	(cmpfunc)0,		  	/*tp_compare*/
	(reprfunc)mxTextSearch_Repr,  	/*tp_repr*/
        0,			  	/*tp_as_number*/
	0,				/*tp_as_number*/
	0,				/*tp_as_mapping*/
	(hashfunc)0,			/*tp_hash*/
	(ternaryfunc)0,			/*tp_call*/
	(reprfunc)0,			/*tp_str*/
	(getattrofunc)0, 		/*tp_getattro*/
	(setattrofunc)0, 		/*tp_setattro*/
};

/* Python Method Table */

statichere
PyMethodDef mxTextSearch_Methods[] =
{   
    Py_MethodListEntry("search",mxTextSearch_search),
    Py_MethodListEntry("find",mxTextSearch_find),
    Py_MethodListEntry("findall",mxTextSearch_findall),
#ifdef COPY_PROTOCOL
    Py_MethodListEntry("__deepcopy__",mxTextSearch_copy),
    Py_MethodListEntry("__copy__",mxTextSearch_copy),
#endif
    {NULL,NULL} /* end of list */
};

/* --- Character Set Object --------------------------------------------*/

staticforward PyMethodDef mxCharSet_Methods[];

/* internal */

/* 8-bit character sets are implemented using a simple 32-byte
   long bitmap with one bit per character.

   Addressing is done as follows:

      def char_is_set(ordinal):
          return bitmap[ordinal >> 3]  & (1 << (ordinal & 7))

*/

#define STRING_CHARSET_SIZE 		256
#define STRING_CHARSET_BITMAP_SIZE 	(STRING_CHARSET_SIZE / 8)

typedef struct {
    unsigned char bitmap[STRING_CHARSET_BITMAP_SIZE];
    						/* character bitmap */
} string_charset;

static
int init_string_charset(mxCharSetObject *cs,
			PyObject *definition)
{
    register Py_ssize_t i, j;
    char *def = PyString_AS_STRING(definition);
    const Py_ssize_t len = PyString_GET_SIZE(definition);
    string_charset *lookup = 0;
    register unsigned char *bitmap;
    int logic = 1;

    /* Handle logic change (first char is '^' for negative matching) */
    if (len > 0 && def[0] == '^') {
	logic = 0;
	i = 1;
    }
    else
	i = 0;
    
    /* Build 32-byte lookup bitmap (one bit per character) */
    lookup = (string_charset *)PyMem_Malloc(sizeof(string_charset));
    if (lookup == NULL) {
	PyErr_NoMemory();
	goto onError;
    }
    memset(lookup, 0, sizeof(string_charset));
    cs->mode = MXCHARSET_8BITMODE;
    cs->lookup = (void *)lookup;
    bitmap = lookup->bitmap;

    for (; i < len; i++) {

	/* Handle escapes: "b\-d", "\\" */
	if (def[i] == '\\') {
	    if (i < len - 1 && def[i+1] == '\\') {
		j = (unsigned char)'\\';
		bitmap[j >> 3] |= 1 << (j & 7);
		i++;
	    }
	    continue;
	}

	/* Handle ranges: "b-d", "\\-z", "\--z" */
	if (i < len - 2 && def[i+1] == '-') {
	    unsigned char range_left = def[i];
	    unsigned char range_right = def[i+2];
	    for (j = range_left; j <= range_right; j++)
		bitmap[j >> 3] |= 1 << (j & 7);
	    i++;
	    continue;
	}

	/* Normal processing */
	j = (unsigned char)def[i];
	bitmap[j >> 3] |= 1 << (j & 7);
    }

    /* Invert bitmap if negative matching is requested */
    if (!logic) {
	DPRINTF("init_string_charset: inverting bitmap\n");
	for (i = 0; i < STRING_CHARSET_BITMAP_SIZE; i++)
	    bitmap[i] ^= 0xFF;
    }

    return 0;

 onError:
    if (lookup)
	PyMem_Free((void *)lookup);
    cs->lookup = 0;
    return -1;
}

#ifdef HAVE_UNICODE

/* Unicode character sets are implemented using two step indexing
   which is a good compromise between lookup speed and memory usage.

   Lookup is done using a variable length array of 32-byte bitmap
   blocks. There can be 256 such blocks. Identical blocks are
   collapsed into a single copy.
   
   Addressing is done as follows:

      def char_is_set(ordinal):
          index = bitmapindex[ordinal >> 8]
	  bitmap = bitmaps[index]
          return bitmap[(ordinal >> 3) & 31]  & (1 << (ordinal & 7))

   The technique used here is very similar to what is done in Python's
   SRE (see the BIGCHARSET patch by Martin von Loewis). Compression
   should be reasonably good since character sets in practice usually
   only contains a few single characters or longer ranges of Unicode
   characters.

*/

#define UNICODE_CHARSET_SIZE 		65536
#define UNICODE_CHARSET_BITMAP_SIZE 	32
#define UNICODE_CHARSET_BITMAPS 	(UNICODE_CHARSET_SIZE / (UNICODE_CHARSET_BITMAP_SIZE * 8))
#define UNICODE_CHARSET_BIGMAP_SIZE	(UNICODE_CHARSET_SIZE / 8)

typedef struct {
    unsigned char bitmapindex[UNICODE_CHARSET_BITMAPS];	
    					/* Index to char bitmaps */
    unsigned char bitmaps[UNICODE_CHARSET_BITMAPS][UNICODE_CHARSET_BITMAP_SIZE];
    					/* Variable length bitmap array */
} unicode_charset;

static
int init_unicode_charset(mxCharSetObject *cs,
			 PyObject *definition)
{
    register Py_ssize_t i, j;
    Py_UNICODE *def = PyUnicode_AS_UNICODE(definition);
    const Py_ssize_t len = PyUnicode_GET_SIZE(definition);
    unicode_charset *lookup = 0;
    unsigned char bigmap[UNICODE_CHARSET_BIGMAP_SIZE];
    Py_ssize_t blocks;
    int logic = 1;

    /* Handle logic change (first char is '^' for negative matching) */
    if (len > 0 && def[0] == '^') {
	logic = 0;
	i = 1;
    }
    else
	i = 0;
    
    /* Build bigmap */
    memset(bigmap, 0, sizeof(bigmap));
    for (; i < len; i++) {

	/* Handle escapes: "b\-d", "\\" */
	if (def[i] == '\\') {
	    if (i < len - 1 && def[i+1] == '\\') {
		j = (int)'\\';
		bigmap[j >> 3] |= 1 << (j & 7);
		i++;
	    }
	    continue;
	}

	/* Handle ranges: "b-d", "\\-z", "\--z" */
	if (i < len - 2 && def[i+1] == '-') {
	    Py_UNICODE range_left = def[i];
	    Py_UNICODE range_right = def[i+2];
	    if (range_right >= UNICODE_CHARSET_SIZE) {
		Py_Error(PyExc_ValueError,
			 "unicode ordinal out of supported range");
	    }
	    for (j = range_left; j <= range_right; j++)
		bigmap[j >> 3] |= 1 << (j & 7);
	    i++;
	    continue;
	}

	/* Normal processing */
	j = def[i];
	if (j >= UNICODE_CHARSET_SIZE) {
	    Py_Error(PyExc_ValueError,
		     "unicode ordinal out of supported range");
	}
	bigmap[j >> 3] |= 1 << (j & 7);
    }

    /* Build lookup table

       XXX Could add dynamic resizing here... probably not worth it
           though, since sizeof(unicode_charset) isn't all that large.

    */
    lookup = (unicode_charset *)PyMem_Malloc(sizeof(unicode_charset));
    if (lookup == NULL) {
	PyErr_NoMemory();
	goto onError;
    }
    blocks = 0;
    for (i = UNICODE_CHARSET_BITMAPS - 1; i >= 0; i--) {
	unsigned char *block = &bigmap[i << 5];
	for (j = blocks - 1; j >= 0; j--)
	    if (memcmp(lookup->bitmaps[j], block, 
		       UNICODE_CHARSET_BITMAP_SIZE) == 0)
		break;
	if (j < 0) {
	    j = blocks;
	    DPRINTF("init_unicode_charset: Creating new block %i for %i\n", 
		    j, i);
	    memcpy(lookup->bitmaps[j], block, UNICODE_CHARSET_BITMAP_SIZE);
	    blocks++;
	}
	else
	    DPRINTF("init_unicode_charset: Reusing block %i for %i\n", j, i);
	lookup->bitmapindex[i] = j;
    }
    DPRINTF("init_unicode_charset: Map size: %i block(s) = %i bytes\n", 
	    blocks, UNICODE_CHARSET_BITMAPS + 
	    blocks * UNICODE_CHARSET_BITMAP_SIZE);
    lookup = (unicode_charset *)PyMem_Realloc(lookup, 
				 UNICODE_CHARSET_BITMAPS 
				 + blocks * UNICODE_CHARSET_BITMAP_SIZE);
    if (lookup == NULL) {
	PyErr_NoMemory();
	goto onError;
    }

    /* Invert bitmaps if negative matching is requested */
    if (!logic) {
	register unsigned char *bitmap = &lookup->bitmaps[0][0];
	DPRINTF("init_unicode_charset: inverting bitmaps\n");
	for (i = 0; i < blocks * UNICODE_CHARSET_BITMAP_SIZE; i++)
	    bitmap[i] ^= 0xFF;
    }

    cs->mode = MXCHARSET_UCS2MODE;
    cs->lookup = (void *)lookup;
    return 0;

 onError:
    if (lookup)
	PyMem_Free((void *)lookup);
    cs->lookup = 0;
    return -1;
}

#endif

/* allocation */

static
PyObject *mxCharSet_New(PyObject *definition)
{
    mxCharSetObject *cs;

    cs = PyObject_NEW(mxCharSetObject, &mxCharSet_Type);
    if (cs == NULL) 
	return NULL;
    Py_INCREF(definition);
    cs->definition = definition;
    cs->lookup = NULL;
    cs->mode = -1;

    if (PyString_Check(definition)) {
	if (init_string_charset(cs, definition))
	    goto onError;
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(definition)) {
	if (init_unicode_charset(cs, definition))
	    goto onError;
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "character set definition must be string or unicode");

    return (PyObject *)cs;

 onError:
    Py_DECREF(cs);
    return NULL;
}

Py_C_Function( mxCharSet_CharSet,
	       "CharSet(definition)\n\n"
	       "Create a character set matching object from the string"
	       )
{
    PyObject *definition;

    Py_GetArg("O:CharSet", definition);
    return mxCharSet_New(definition);

 onError:
    return NULL;
}

static 
void mxCharSet_Free(mxCharSetObject *cs)
{
    Py_XDECREF(cs->definition);
    if (cs->lookup)
	PyMem_Free(cs->lookup);
    PyObject_Del(cs);
}

/* C APIs */

#define cs ((mxCharSetObject *)self)

int mxCharSet_ContainsChar(PyObject *self,
			   register unsigned char ch)
{
    if (!mxCharSet_Check(self)) {
	PyErr_BadInternalCall();
	goto onError;
    }
    
    if (cs->mode == MXCHARSET_8BITMODE) {
	unsigned char *bitmap = ((string_charset *)cs->lookup)->bitmap;
	return ((bitmap[ch >> 3] & (1 << (ch & 7))) != 0);
    }
#ifdef HAVE_UNICODE
    else if (cs->mode == MXCHARSET_UCS2MODE) {
	unicode_charset *lookup = (unicode_charset *)cs->lookup;
	unsigned char *bitmap = lookup->bitmaps[lookup->bitmapindex[0]];
	return ((bitmap[ch >> 3] & (1 << (ch & 7))) != 0);
    }
#endif
    else {
	Py_Error(mxTextTools_Error,
		 "unsupported character set mode");
    }

 onError:
    return -1;
}

#ifdef HAVE_UNICODE

int mxCharSet_ContainsUnicodeChar(PyObject *self,
				  register Py_UNICODE ch)
{
    if (!mxCharSet_Check(self)) {
	PyErr_BadInternalCall();
	goto onError;
    }
    
    if (cs->mode == MXCHARSET_8BITMODE) {
	unsigned char *bitmap = ((string_charset *)cs->lookup)->bitmap;
	if (ch >= 256)
	    return 0;
	return ((bitmap[ch >> 3] & (1 << (ch & 7))) != 0);
    }
    else if (cs->mode == MXCHARSET_UCS2MODE) {
	unicode_charset *lookup = (unicode_charset *)cs->lookup;
	unsigned char *bitmap = lookup->bitmaps[lookup->bitmapindex[ch >> 8]];
	return ((bitmap[(ch >> 3) & 31] & (1 << (ch & 7))) != 0);
    }
    else {
	Py_Error(mxTextTools_Error,
		 "unsupported character set mode");
    }

 onError:
    return -1;
}

#endif

static
int mxCharSet_Contains(PyObject *self,
		       PyObject *other)
{
    if (PyString_Check(other)) {
	Py_Assert(PyString_GET_SIZE(other) == 1,
		  PyExc_TypeError,
		  "expected a single character");
	return mxCharSet_ContainsChar(self, PyString_AS_STRING(other)[0]);
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(other)) {
	Py_Assert(PyUnicode_GET_SIZE(other) == 1,
		  PyExc_TypeError,
		  "expected a single unicode character");
	return mxCharSet_ContainsUnicodeChar(self, 
					     PyUnicode_AS_UNICODE(other)[0]);
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode character");

 onError:
    return -1;
}

/* In mode 1, find the position of the first character in text
   belonging to set. This may also be stop or start-1 in case no such
   character is found during the search (depending on the direction).

   In mode 0, find the first character not in set. This may also be
   stop or start-1 in case no such character is found during the
   search (depending on the direction).

   The search is done in the slice start:stop.

   -2 is returned in case of an error.

*/

static
int mxCharSet_FindChar(PyObject *self,
		       unsigned char *text,
		       Py_ssize_t start,
		       Py_ssize_t stop,
		       const int mode,
		       const int direction)
{
    register Py_ssize_t i;
    register unsigned int c;
    register unsigned int block;
    unsigned char *bitmap;

    if (!mxCharSet_Check(self)) {
	PyErr_BadInternalCall();
	goto onError;
    }
    
    if (cs->mode == MXCHARSET_8BITMODE)
	bitmap = ((string_charset *)cs->lookup)->bitmap;
#ifdef HAVE_UNICODE
    else if (cs->mode == MXCHARSET_UCS2MODE) {
	unicode_charset *lookup = (unicode_charset *)cs->lookup;
	bitmap = lookup->bitmaps[lookup->bitmapindex[0]];
    }
#endif
    else {
	Py_Error(mxTextTools_Error,
		 "unsupported character set mode");
    }

    if (direction > 0) {
	if (mode)
	    /* Find first char in set */
	    for (i = start; i < stop; i++) {
		c = text[i];
		block = bitmap[c >> 3];
		if (block && ((block & (1 << (c & 7))) != 0))
		    break;
	    }
	else
	    /* Find first char not in set */
	    for (i = start; i < stop; i++) {
		c = text[i];
		block = bitmap[c >> 3];
		if (!block || ((block & (1 << (c & 7))) == 0))
		    break;
	    }
    }
    else {
	if (mode)
	    /* Find first char in set, searching from the end */
	    for (i = stop - 1; i >= start; i--) {
		c = text[i];
		block = bitmap[c >> 3];
		if (block && ((block & (1 << (c & 7))) != 0))
		    break;
	    }
	else
	    /* Find first char not in set, searching from the end */
	    for (i = stop - 1; i >= start; i--) {
		c = text[i];
		block = bitmap[c >> 3];
		if (!block || ((block & (1 << (c & 7))) == 0))
		    break;
	    }
    }
    return i;

 onError:
    return -2;
}

#ifdef HAVE_UNICODE

static
int mxCharSet_FindUnicodeChar(PyObject *self,
			      Py_UNICODE *text,
			      Py_ssize_t start,
			      Py_ssize_t stop,
			      const int mode,
			      const int direction)
{
    register int i;
    register unsigned int c;
    register unsigned int block;
    unsigned char *bitmap;

    if (!mxCharSet_Check(self)) {
	PyErr_BadInternalCall();
	goto onError;
    }
    
    if (cs->mode == MXCHARSET_8BITMODE) {
	bitmap = ((string_charset *)cs->lookup)->bitmap;
	if (direction > 0) {
	    if (mode)
		/* Find first char in set */
		for (i = start; i < stop; i++) {
		    c = text[i];
		    if (c > 256)
			continue;
		    block = bitmap[c >> 3];
		    if (block && ((block & (1 << (c & 7))) != 0))
			break;
		}
	    else
		/* Find first char not in set */
		for (i = start; i < stop; i++) {
		    c = text[i];
		    if (c > 256)
			break;
		    block = bitmap[c >> 3];
		    if (!block || ((block & (1 << (c & 7))) == 0))
			break;
		}
        }
	else {
	    if (mode)
		/* Find first char in set, searching from the end */
		for (i = stop - 1; i >= start; i--) {
		    c = text[i];
		    if (c > 256)
			continue;
		    block = bitmap[c >> 3];
		    if (block && ((block & (1 << (c & 7))) != 0))
			break;
		}
	    else
		/* Find first char not in set, searching from the end */
		for (i = stop - 1; i >= start; i--) {
		    c = text[i];
		    if (c > 256)
			break;
		    block = bitmap[c >> 3];
		    if (!block || ((block & (1 << (c & 7))) == 0))
			break;
		}
	}
	return i;
    }

#ifdef HAVE_UNICODE
    else if (cs->mode == MXCHARSET_UCS2MODE) {
	unicode_charset *lookup = (unicode_charset *)cs->lookup;
	if (direction > 0) {
	    if (mode)
		/* Find first char in set */
		for (i = start; i < stop; i++) {
		    c = text[i];
		    bitmap = lookup->bitmaps[lookup->bitmapindex[c >> 8]];
		    block = bitmap[(c >> 3) & 31];
		    if (block && ((block & (1 << (c & 7))) != 0))
			break;
		}
	    else
		/* Find first char not in set */
		for (i = start; i < stop; i++) {
		    c = text[i];
		    bitmap = lookup->bitmaps[lookup->bitmapindex[c >> 8]];
		    block = bitmap[(c >> 3) & 31];
		    if (!block || ((block & (1 << (c & 7))) == 0))
			break;
		}
	}
	else {
	    if (mode)
		/* Find first char in set, searching from the end */
		for (i = stop - 1; i >= start; i--) {
		    c = text[i];
		    bitmap = lookup->bitmaps[lookup->bitmapindex[c >> 8]];
		    block = bitmap[(c >> 3) & 31];
		    if (block && ((block & (1 << (c & 7))) != 0))
			break;
		}
	    else
		/* Find first char not in set, searching from the end */
		for (i = stop - 1; i >= start; i--) {
		    c = text[i];
		    bitmap = lookup->bitmaps[lookup->bitmapindex[c >> 8]];
		    block = bitmap[(c >> 3) & 31];
		    if (!block || ((block & (1 << (c & 7))) == 0))
			break;
		}
	}
	return i;
    }
#endif
    else {
	Py_Error(mxTextTools_Error,
		 "unsupported character set mode");
    }

 onError:
    return -2;
}

#endif

/* Return the position of the first character in text[start:stop]
   occurring in set or -1 in case no such character exists.

*/

static
int mxCharSet_Search(PyObject *self,
		     PyObject *text,
		     Py_ssize_t start,
		     Py_ssize_t stop,
		     int direction)
{
    Py_ssize_t position;
    
    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, stop);
	position = mxCharSet_FindChar(self, 
				      (unsigned char *)PyString_AS_STRING(text),
				      start,
				      stop,
				      1,
				      direction);
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, start, stop);
	position = mxCharSet_FindUnicodeChar(self,
					     PyUnicode_AS_UNICODE(text),
					     start,
					     stop,
					     1,
					     direction);
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");

    if ((direction > 0 && position >= stop) ||
	(direction <= 0 && position < start))
	position = -1;
    return position;

 onError:
    return -2;
}

/* Return the longest match of characters from set in
   text[start:stop]. 

   If direction is positive, the search is done from the left (longest
   prefix), otherwise it is started from the right (longest suffix).

   -1 is returned in case of an error.

*/

Py_ssize_t mxCharSet_Match(PyObject *self,
		    PyObject *text,
		    Py_ssize_t start,
		    Py_ssize_t stop,
		    int direction)
{
    Py_ssize_t position;
    
    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, stop);
	position = mxCharSet_FindChar(self, 
				      (unsigned char *)PyString_AS_STRING(text),
				      start,
				      stop,
				      0,
				      direction);
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, start, stop);
	position = mxCharSet_FindUnicodeChar(self,
					     PyUnicode_AS_UNICODE(text),
					     start,
					     stop,
					     0,
					     direction);
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");

    if (position < -1)
	goto onError;
    if (direction > 0)
	return position - start;
    else
	return stop-1 - position;

 onError:
    return -1;
}

/* Stips off characters appearing in the character set from text[start:stop]
   and returns the result as Python string object.

   where indicates the mode:
   where < 0: strip left only
   where = 0: strip left and right
   where > 0: strip right only

*/
static
PyObject *mxCharSet_Strip(PyObject *self,
			  PyObject *text,
			  Py_ssize_t start,
			  Py_ssize_t stop,
			  Py_ssize_t where)
{
    Py_ssize_t left,right;
    
    if (!mxCharSet_Check(self)) {
	PyErr_BadInternalCall();
	goto onError;
    }

    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, stop);

	/* Strip left */
	if (where <= 0) {
	    left = mxCharSet_FindChar(self, 
				      (unsigned char *)PyString_AS_STRING(text),
				      start,
				      stop,
				      0,
				      1);
	    if (left < 0)
		goto onError;
	}
	else
	    left = start;

	/* Strip right */
	if (where >= 0) {
	    right = mxCharSet_FindChar(self, 
				       (unsigned char *)PyString_AS_STRING(text),
				       left,
				       stop,
				       0,
				       -1) + 1;
	    if (right < 0)
		goto onError;
	}
	else
	    right = stop;

	return PyString_FromStringAndSize(PyString_AS_STRING(text) + left, 
					  max(right - left, 0));
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
        Py_CheckUnicodeSlice(text, start, stop);

	/* Strip left */
	if (where <= 0) {
	    left = mxCharSet_FindUnicodeChar(self, 
					     PyUnicode_AS_UNICODE(text),
					     start,
					     stop,
					     0,
					     1);
	    if (left < 0)
		goto onError;
	}
	else
	    left = start;

	/* Strip right */
	if (where >= 0) {
	    right = mxCharSet_FindUnicodeChar(self, 
					     PyUnicode_AS_UNICODE(text),
					     start,
					     stop,
					     0,
					     -1) + 1;
	    if (right < 0)
		goto onError;
	}
	else
	    right = stop;
	
	return PyUnicode_FromUnicode(PyUnicode_AS_UNICODE(text) + left, 
				     max(right - left, 0));
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");

 onError:
    return NULL;
}

static 
PyObject *mxCharSet_Split(PyObject *self,
			  PyObject *text,
			  Py_ssize_t start,
			  Py_ssize_t text_len,
			  int include_splits)
{
    PyObject *list = NULL;
    PyObject *s;
    register Py_ssize_t x;
    Py_ssize_t listitem = 0;
    Py_ssize_t listsize = INITIAL_LIST_SIZE;

    if (!mxCharSet_Check(self)) {
	PyErr_BadInternalCall();
	goto onError;
    }
    
    list = PyList_New(listsize);
    if (!list)
	goto onError;

    if (PyString_Check(text)) {
	unsigned char *tx = (unsigned char *)PyString_AS_STRING(text);

	Py_CheckStringSlice(text, start, text_len);

	x = start;
	while (x < text_len) {
	    Py_ssize_t z;

	    /* Skip all text in set (include_splits == 0), not in set
    	       (include_splits == 1) */
	    z = x;
	    x = mxCharSet_FindChar(self, tx, x, text_len, include_splits, 1);

	    /* Append the slice to list */
	    if (include_splits) {
		s = PyString_FromStringAndSize((char *)&tx[z], x - z);
		if (!s)
		    goto onError;
		if (listitem < listsize)
		    PyList_SET_ITEM(list,listitem,s);
		else {
		    PyList_Append(list,s);
		    Py_DECREF(s);
		}
		listitem++;

		if (x >= text_len)
		    break;
	    }
	    
	    /* Skip all text in set (include_splits == 1), not in set
    	       (include_splits == 0) */
	    z = x;
	    x = mxCharSet_FindChar(self, tx, x, text_len, !include_splits, 1);

	    /* Append the slice to list if it is not empty */
	    if (x > z) {
		s = PyString_FromStringAndSize((char *)&tx[z], x - z);
		if (!s)
		    goto onError;
		if (listitem < listsize)
		    PyList_SET_ITEM(list,listitem,s);
		else {
		    PyList_Append(list,s);
		    Py_DECREF(s);
		}
		listitem++;
	    }
	}
	
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_UNICODE *tx = PyUnicode_AS_UNICODE(text);

	Py_CheckUnicodeSlice(text, start, text_len);

	x = start;
	while (x < text_len) {
	    Py_ssize_t z;

	    /* Skip all text in set (include_splits == 0), not in set
    	       (include_splits == 1) */
	    z = x;
	    x = mxCharSet_FindUnicodeChar(self, tx, x, text_len, include_splits, 1);

	    /* Append the slice to list */
	    if (include_splits) {
		s = PyUnicode_FromUnicode(&tx[z], x - z);
		if (!s)
		    goto onError;
		if (listitem < listsize)
		    PyList_SET_ITEM(list,listitem,s);
		else {
		    PyList_Append(list,s);
		    Py_DECREF(s);
		}
		listitem++;

		if (x >= text_len)
		    break;
	    }
	    
	    /* Skip all text in set (include_splits == 1), not in set
    	       (include_splits == 0) */
	    z = x;
	    x = mxCharSet_FindUnicodeChar(self, tx, x, text_len, !include_splits, 1);

	    /* Append the slice to list if it is not empty */
	    if (x > z) {
		s = PyUnicode_FromUnicode(&tx[z], x - z);
		if (!s)
		    goto onError;
		if (listitem < listsize)
		    PyList_SET_ITEM(list,listitem,s);
		else {
		    PyList_Append(list,s);
		    Py_DECREF(s);
		}
		listitem++;
	    }
	}
    }
#endif    
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    
    /* Resize list if necessary */
    if (listitem < listsize)
	PyList_SetSlice(list, listitem, listsize, (PyObject*)NULL);

    return list;
    
 onError:
    Py_XDECREF(list);
    return NULL;
}

/* methods */

Py_C_Function( mxCharSet_contains,
	       ".contains(char)\n\n"
	       )
{
    PyObject *chr;
    int rc;

    Py_GetArg("O:CharSet.contains", chr);
    
    rc = mxCharSet_Contains(self, chr);
    if (rc < 0)
	goto onError;
    return PyInt_FromLong(rc);

 onError:
    return NULL;
}

Py_C_Function( mxCharSet_search,
	       ".search(text[, direction=1, start=0, stop=len(text)])\n\n"
	       )
{
    PyObject *text;
    int direction = 1;
    Py_ssize_t start = 0, stop = INT_MAX;
    int rc;

    Py_Get4Args("O|iii:CharSet.search", text, direction, start, stop);
    
    rc = mxCharSet_Search(self, text, start, stop, direction);
    if (rc == -1)
	Py_ReturnNone();
    if (rc < -1)
	goto onError;
    return PyInt_FromLong(rc);

 onError:
    return NULL;
}

Py_C_Function( mxCharSet_match,
	       ".match(text[, direction=1, start=0, stop=len(text)])\n\n"
	       )
{
    PyObject *text;
    int direction = 1;
    Py_ssize_t start = 0, stop = INT_MAX;
    int rc;

    Py_Get4Args("O|iii:CharSet.match", text, direction, start, stop);
    
    rc = mxCharSet_Match(self, text, start, stop, direction);
    if (rc < 0)
	goto onError;
    return PyInt_FromLong(rc);

 onError:
    return NULL;
}

Py_C_Function( mxCharSet_split,
	       ".split(text[, start=0, stop=len(text)])\n\n"
	       )
{
    PyObject *text;
    Py_ssize_t start = 0, stop = INT_MAX;

    Py_Get3Args("O|ii:CharSet.split", text, start, stop);
    
    return mxCharSet_Split(self, text, start, stop, 0);

 onError:
    return NULL;
}

Py_C_Function( mxCharSet_splitx,
	       ".splitx(text[, start=0, stop=len(text)])\n\n"
	       )
{
    PyObject *text;
    Py_ssize_t start = 0, stop = INT_MAX;

    Py_Get3Args("O|ii:CharSet.splitx", text, start, stop);
    
    return mxCharSet_Split(self, text, start, stop, 1);

 onError:
    return NULL;
}

Py_C_Function( mxCharSet_strip,
	       ".strip(text[, where=0, start=0, stop=len(text)])\n\n"
	       )
{
    PyObject *text;
    Py_ssize_t where = 0;
    Py_ssize_t start = 0, stop = INT_MAX;

    Py_Get4Args("O|iii:CharSet.strip", text, where, start, stop);
    
    return mxCharSet_Strip(self, text, start, stop, where);

 onError:
    return NULL;
}

#ifdef COPY_PROTOCOL
Py_C_Function( mxCharSet_copy,
	       "copy([memo])\n\n"
	       "Return a new reference for the instance. This function\n"
	       "is used for the copy-protocol. Real copying doesn't take\n"
	       "place, since the instances are immutable.")
{
    PyObject *memo;
    
    Py_GetArg("|O",memo);
    Py_INCREF(cs);
    return (PyObject *)cs;
 onError:
    return NULL;
}
#endif

#undef cs

/* --- slots --- */

static
PyObject *mxCharSet_Repr(mxCharSetObject *self)
{
    PyObject *v;
    char t[500], *reprstr;

    v = PyObject_Repr(self->definition);
    if (v == NULL)
	return NULL;
    reprstr = PyString_AsString(v);
    if (reprstr == NULL)
	return NULL;
    sprintf(t, "<Character Set object for %.400s at 0x%lx>",
	    reprstr, (long)self);
    Py_DECREF(v);
    return PyString_FromString(t);
}

static 
PyObject *mxCharSet_GetAttr(mxCharSetObject *self,
			    char *name)
{
    PyObject *v;
    
    if (Py_WantAttr(name,"definition")) {
	v = self->definition;
	Py_INCREF(v);
	return v;
    }

    else if (Py_WantAttr(name,"__members__"))
	return Py_BuildValue("[s]",
			     "definition");
    
    return Py_FindMethod(mxCharSet_Methods, (PyObject *)self, (char *)name);
}

/* Python Type Tables */

static
PySequenceMethods mxCharSet_TypeAsSequence = {
    (lenfunc)0,				/*sq_length*/
    (binaryfunc)0,			/*sq_concat*/
    (ssizeargfunc)0,			/*sq_repeat*/
    (ssizeargfunc)0,			/*sq_item*/
    (ssizessizeargfunc)0,			/*sq_slice*/
    (ssizeobjargproc)0,			/*sq_ass_item*/
    (ssizessizeobjargproc)0,		/*sq_ass_slice*/
#if PY_VERSION_HEX >= 0x02000000
    (objobjproc)mxCharSet_Contains,     /*sq_contains*/
#endif
};

PyTypeObject mxCharSet_Type = {
        PyObject_HEAD_INIT(0)		/* init at startup ! */
	0,			  	/* ob_size */
	"Character Set",	  	/* tp_name */
	sizeof(mxCharSetObject),	/* tp_basicsize */
	0,			  	/* tp_itemsize */
	/* methods */
	(destructor)mxCharSet_Free,	/* tp_dealloc */
	(printfunc)0,			/* tp_print */
	(getattrfunc)mxCharSet_GetAttr, /* tp_getattr */
	(setattrfunc)0,		  	/* tp_setattr */
	(cmpfunc)0,		  	/* tp_compare */
	(reprfunc)mxCharSet_Repr,  	/* tp_repr */
        0,			  	/* tp_as_number */
        &mxCharSet_TypeAsSequence,	/* tp_as_sequence */
	0,				/* tp_as_mapping */
	(hashfunc)0,			/* tp_hash */
	(ternaryfunc)0,			/* tp_call */
	(reprfunc)0,			/* tp_str */
	(getattrofunc)0, 		/* tp_getattro */
	(setattrofunc)0, 		/* tp_setattro */
        0,				/* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,		/* tp_flags */
        (char*) 0,			/* tp_doc */
};

/* Python Method Table */

statichere
PyMethodDef mxCharSet_Methods[] =
{   
    Py_MethodListEntry("contains",mxCharSet_contains),
    Py_MethodListEntry("search",mxCharSet_search),
    Py_MethodListEntry("match",mxCharSet_match),
    Py_MethodListEntry("strip",mxCharSet_strip),
    Py_MethodListEntry("split",mxCharSet_split),
    Py_MethodListEntry("splitx",mxCharSet_splitx),
#ifdef COPY_PROTOCOL
    Py_MethodListEntry("__deepcopy__",mxCharSet_copy),
    Py_MethodListEntry("__copy__",mxCharSet_copy),
#endif
    {NULL,NULL} /* end of list */
};

/* --- Tag Table Object ------------------------------------------------*/

staticforward PyMethodDef mxTagTable_Methods[];

PyObject *mxTagTable_New(PyObject *definition,
			 int tabletype,
			 int cacheable);

/* internal APIs */

static
PyObject *tc_get_item(register PyObject *obj,
		      register Py_ssize_t i)
{
    if (PyTuple_Check(obj)) {
	if (i > PyTuple_GET_SIZE(obj))
	    return NULL;
	return PyTuple_GET_ITEM(obj, i);
    }
    else if (PyList_Check(obj)) {
	if (i > PyList_GET_SIZE(obj))
	    return NULL;
	return PyList_GET_ITEM(obj, i);
    }
    else 
	return NULL;
}

static
Py_ssize_t tc_length(register PyObject *obj)
{
    if (obj == NULL)
	return -1;
    else if (PyTuple_Check(obj))
	return PyTuple_GET_SIZE(obj);
    else if (PyList_Check(obj))
	return PyList_GET_SIZE(obj);
    else 
	return -1;
}

/* Add a jump target to the jump dictionary */

static
Py_ssize_t tc_add_jumptarget(PyObject *jumpdict,
		      PyObject *targetname,
		      Py_ssize_t index)
{
    PyObject *v;
    
    v = PyDict_GetItem(jumpdict, targetname);
    if (v != NULL)
	Py_ErrorWithArg(PyExc_TypeError,
			"tag table entry %d: "
			"jump target already defined", (unsigned int) index);
    v = PyInt_FromLong(index);
    if (v == NULL)
	goto onError;
    if (PyDict_SetItem(jumpdict, targetname, v))
	goto onError;
    Py_DECREF(v);
    return 0;
    
 onError:
    return -1;
}

/* Convert a string command argument to either an 8-bit string or
   Unicode depending on the tabletype. */

static
PyObject *tc_convert_string_arg(PyObject *arg,
				Py_ssize_t tableposition,
				int tabletype)
{
    /* Convert to strings */
    if (tabletype == MXTAGTABLE_STRINGTYPE) {
	if (PyString_Check(arg))
	    return arg;
#ifdef HAVE_UNICODE
	else if (PyUnicode_Check(arg)) {
	    Py_DECREF(arg);
	    arg = PyUnicode_AsEncodedString(arg,
					    NULL,
					    NULL);
	    if (arg == NULL)
		Py_ErrorWithArg(PyExc_TypeError,
				"tag table entry %d: "
				"conversion from Unicode to "
				"string failed", (unsigned int)tableposition);
	}
#endif
	else
	    Py_ErrorWithArg(PyExc_TypeError,
			    "tag table entry %d: "
			    "command argument must be a "
			    "string or unicode", (unsigned int)tableposition);
    }

#ifdef HAVE_UNICODE
    /* Convert to Unicode */
    else if (tabletype == MXTAGTABLE_UNICODETYPE) {
	if (PyUnicode_Check(arg))
	    return arg;
	else if (PyString_Check(arg)) {
	    Py_DECREF(arg);
	    arg = PyUnicode_Decode(PyString_AS_STRING(arg),
				    PyString_GET_SIZE(arg),
				    NULL,
				    NULL);
	    if (arg == NULL)
		Py_ErrorWithArg(PyExc_TypeError,
				"tag table entry %d: "
				"conversion from string to "
				"Unicode failed", (unsigned int)tableposition);
	}
	else
	    Py_ErrorWithArg(PyExc_TypeError,
			    "tag table entry %d: "
			    "command argument must be a "
			    "string or unicode", (unsigned int)tableposition);
    }
#endif

    else
	Py_Error(mxTextTools_Error,
		 "unsupported table type");

    return arg;
    
 onError:
    return NULL;
}

/* Cleanup any references in the tag table. */

static
int tc_cleanup(mxTagTableObject *tagtable)
{
    Py_ssize_t i;
    for (i = 0; i < tagtable->ob_size; i++) {
	mxTagTableEntry *tagtableentry = &tagtable->entry[i];

	Py_XDECREF(tagtableentry->tagobj);
	tagtableentry->tagobj = NULL;
	Py_XDECREF(tagtableentry->args);
	tagtableentry->args = NULL;
    }
    return 0;
}

/* Initialize the tag table (this is the actual Tag Table compiler) */

static
int init_tag_table(mxTagTableObject *tagtable,
		   PyObject *table,
		   Py_ssize_t size,
		   int tabletype,
		   int cacheable)
{
    Py_ssize_t i;
    PyObject *entry;
    Py_ssize_t entry_len;
    PyObject *tagobj, *command, *args = 0, *je, *jne;
    PyObject *jumpdict, *v;
    int secondpass, own_args = 0;

    jumpdict = PyDict_New();
    if (jumpdict == NULL)
	return -1;

    /* Reset to all fields to 0 */
    memset(&tagtable->entry[0], 0, size * sizeof(mxTagTableEntry));
    
    /* First pass */
    secondpass = 0;
    for (i = 0; i < size; i++) {
	mxTagTableEntry *tagtableentry = &tagtable->entry[i];

	/* Get table entry i and parse it */
	entry = tc_get_item(table, i);
	if (entry == NULL) {
	    Py_ErrorWithArg(PyExc_TypeError,
			    "tag table entry %d: "
			    "not found or not a supported entry type", (unsigned int)i);
	}

	/* Special handling for jump marks (args is set to the jump
	   mark string, jump target index is the next table entry) */
	if (PyString_Check(entry)) {
	    if (tc_add_jumptarget(jumpdict, entry, i + 1))
		goto onError;
	    tagtableentry->tagobj = NULL;
	    tagtableentry->cmd = MATCH_JUMPTARGET;
	    tagtableentry->flags = 0;
	    Py_INCREF(entry);
	    tagtableentry->args = entry;
	    tagtableentry->jne = 0;
	    tagtableentry->je = 1;
	    continue;
	}

	/* Get entry length */
	entry_len = tc_length(entry);
	if (entry_len < 3) {
	    Py_ErrorWithArg(PyExc_TypeError,
			    "tag table entry %d: "
			    "expected an entry of the form "
			    "(tagobj,command,arg[,jne[,je]])", (unsigned int)i);
	}

	/* Decode entry parts: (tagobj, command, args[, jne[, je]]) */
	tagobj = tc_get_item(entry, 0);
	command = tc_get_item(entry, 1);
	args = tc_get_item(entry, 2);
	if (entry_len >= 4)
	    jne = tc_get_item(entry, 3);
	else
	    jne = NULL;
	if (entry_len >= 5) 
	    je = tc_get_item(entry, 4);
	else
	    je = NULL;

	if (tagobj == NULL ||
	    command == NULL ||
	    args == NULL ||
	    (entry_len >= 4 && jne == NULL) ||
	    (entry_len >= 5 && je == NULL)) {
	    Py_ErrorWithArg(PyExc_TypeError,
			    "tag table entry %d: "
			    "expected an entry of the form "
			    "(tagobj,command,arg[,jne[,je]])",(unsigned int) i);
	}

	/* Store tagobj, None gets converted to NULL */
	if (tagobj != Py_None)
	    Py_INCREF(tagobj);
	else
	    tagobj = NULL;
	tagtableentry->tagobj = tagobj;

	/* Decode command and flags */
	Py_AssertWithArg(PyInt_Check(command),
			 PyExc_TypeError,
			 "tag table entry %d: "
			 "command must be an integer",(unsigned int)i);
	tagtableentry->cmd = PyInt_AS_LONG(command) & 0xFF;
	tagtableentry->flags = PyInt_AS_LONG(command) - tagtableentry->cmd;

	/* Check command arguments */
	Py_INCREF(args);
	own_args = 1;

	switch (tagtableentry->cmd) {

	case MATCH_JUMP: /* == MATCH_FAIL */
	case MATCH_EOF:
	case MATCH_LOOP:
	    /* args is ignored */
	    break;
	
	case MATCH_SKIP:
	case MATCH_MOVE:
	case MATCH_LOOPCONTROL:
	    Py_AssertWithArg(PyInt_Check(args),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "Skip|Move|LoopControl command argument "
			     "must be an integer", (unsigned int)i);
	    break;
	
	case MATCH_JUMPTARGET:
	    Py_AssertWithArg(PyString_Check(args),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "JumpMark command argument must be a string",(unsigned int)i);
	    if (tc_add_jumptarget(jumpdict, args, i + 1))
		goto onError;
	    break;

	case MATCH_ALLIN:
	case MATCH_ALLNOTIN:
	case MATCH_IS:
	case MATCH_ISIN:
	case MATCH_ISNOTIN:
	case MATCH_WORD:
	case MATCH_WORDSTART:
	case MATCH_WORDEND:
	    args = tc_convert_string_arg(args, i, tabletype);
	    if (args == NULL)
		goto onError;
	    break;

	case MATCH_ALLINSET:
	case MATCH_ISINSET:
	    Py_AssertWithArg(PyString_Check(args) && 
			     PyString_GET_SIZE(args) == 32,
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "AllInSet|IsInSet command argument must "
			     "be a set() string",(unsigned int)i);
	    break;

	case MATCH_ALLINCHARSET:
	case MATCH_ISINCHARSET:
	    Py_AssertWithArg(mxCharSet_Check(args),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "AllInCharSet|IsInCharSet command argument must "
			     "be a CharSet instance",(unsigned int)i);
	    break;

	case MATCH_SWORDSTART: /* == MATCH_NOWORD */
	case MATCH_SWORDEND:
	case MATCH_SFINDWORD:
	    Py_AssertWithArg(mxTextSearch_Check(args),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "sWordStart|sWordEnd|sFindWord command "
			     "argument must be a TextSearch search "
			     "object",(unsigned int)i);
	    break;
	
	case MATCH_TABLE:
	case MATCH_SUBTABLE:
	    Py_AssertWithArg(mxTagTable_Check(args) ||
			     PyTuple_Check(args) ||
			     PyList_Check(args) ||
			     (PyInt_Check(args) && 
			      PyInt_AS_LONG(args) == MATCH_THISTABLE),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "Table|SubTable command argument "
			     "must be a tag table tuple/object or "
			     "ThisTable", (unsigned int)i);
	    /* XXX We shouldn't recursively compile tag table tuples here
		   because this will slow down the compile process
		   too much and it's not clear whether this particular
		   table will ever be used during tagging.
	    */
	    if (!mxTagTable_Check(args) && !PyInt_Check(args)) {
		Py_DECREF(args);
		args = mxTagTable_New(args, tabletype, cacheable);
		if (args == NULL)
		    goto onError;
	    }
	    break;
	
	case MATCH_TABLEINLIST:
	case MATCH_SUBTABLEINLIST:
	    Py_AssertWithArg(PyTuple_Check(args) &&
			     PyTuple_GET_SIZE(args) == 2 &&
			     PyList_Check(PyTuple_GET_ITEM(args, 0)) &&
			     PyInt_Check(PyTuple_GET_ITEM(args, 1)),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "TableInList|SubTableInList command argument "
			     "must be a 2-tuple (list, integer)",
			     (unsigned int)i);
	    break;

	case MATCH_CALL:
	    Py_AssertWithArg(PyCallable_Check(args),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "Call command argument "
			     "must be a callable object",
			     (unsigned int)i);
	    break;

	case MATCH_CALLARG:
	    Py_AssertWithArg(PyTuple_Check(args) &&
			     PyTuple_GET_SIZE(args) > 0 &&
			     PyCallable_Check(PyTuple_GET_ITEM(args, 0)),
			     PyExc_TypeError,
			     "tag table entry %d: "
			     "CallArg command argument "
			     "must be a tuple (fct,[arg0,arg1,...])",
			     (unsigned int)i);
	    break;
	    
	default:
	    Py_ErrorWith2Args(PyExc_TypeError,
			      "tag table entry %d: "
			      "unknown command integer: %i", 
			      (unsigned int)i, tagtableentry->cmd);
	
	}

	/* Store command args */
	tagtableentry->args = args;
	own_args = 0;

	/* Decode jump offsets */
	if (jne) {
	    if (PyInt_Check(jne))
		tagtableentry->jne = PyInt_AS_LONG(jne);
	    else if (PyString_Check(jne)) {
		/* Mark for back-patching */
		tagtableentry->jne = -424242;
		secondpass = 1;
	    }
	    else
		Py_ErrorWithArg(PyExc_TypeError,
				"tag table entry %d: "
				"jne must be an integer or string", (unsigned int)i);
	}
	else
	    tagtableentry->jne = 0;

	if (je) {
	    if (PyInt_Check(je))
		tagtableentry->je = PyInt_AS_LONG(je);
	    else if (PyString_Check(je)) {
		/* Mark for back-patching */
		tagtableentry->je = -424242;
		secondpass = 1;
	    }
	    else
		Py_ErrorWithArg(PyExc_TypeError,
				"tag table entry %d: "
				"je must be an integer or string", (unsigned int)i);
	}
	else
	    tagtableentry->je = 1;
    }

    /* Second pass (needed to patch string jump targets) */
    if (secondpass)
	for (i = 0; i < size; i++) {
	    mxTagTableEntry *tagtableentry = &tagtable->entry[i];

	    if (tagtableentry->je != -424242 &&
		tagtableentry->jne != -424242)
		continue;

	    /* Entry (most probably) needs back-patching */
	    entry = tc_get_item(table, i);
	    if (entry == NULL) {
		Py_ErrorWithArg(PyExc_TypeError,
				"tag table entry %d: "
				"unexpected error (not found)", (unsigned int)i);
	    }

	    /* Get entry length */
	    entry_len = tc_length(entry);
	    if (entry_len < 0) {
		Py_ErrorWithArg(PyExc_TypeError,
				"tag table entry %d: "
				"unexpected error (no length)", (unsigned int)i);
	    }

	    /* Decode jump offsets */
	    if (entry_len >= 4)
		jne = tc_get_item(entry, 3);
	    else
		jne = NULL;
	    if (entry_len >= 5) 
		je = tc_get_item(entry, 4);
	    else
		je = NULL;

	    /* Patch jump offsets */
	    if (jne && PyString_Check(jne)) {
		v = PyDict_GetItem(jumpdict, jne);
		if (v == NULL || !PyInt_Check(v))
		    Py_ErrorWith2Args(PyExc_TypeError,
				      "tag table entry %d: "
				      "jne jump target '%s' not found", 
				      (unsigned int)i, PyString_AS_STRING(jne));
		tagtableentry->jne = PyInt_AS_LONG(v) - i;
	    }
	    if (je && PyString_Check(je)) {
		v = PyDict_GetItem(jumpdict, je);
		if (v == NULL || !PyInt_Check(v))
		    Py_ErrorWith2Args(PyExc_TypeError,
				      "tag table entry %d: "
				      "je jump target '%s' not found", 
				      (unsigned int)i, PyString_AS_STRING(je));
		tagtableentry->je = PyInt_AS_LONG(v) - i;
	    }
	}
    
    Py_DECREF(jumpdict);
    return 0;

 onError:
    if (own_args) {
	Py_DECREF(args);
    }
    return -1;
}

/* Check the cache for an already compiled TagTable for this
   definition.  Return NULL in case of an error, Py_None without
   INCREF in case no such table was found or the TagTable object. */

static
PyObject *consult_tagtable_cache(PyObject *definition,
				 int tabletype,
				 int cacheable)
{
    PyObject *v, *key, *tt;

    if (!PyTuple_Check(definition) || !cacheable)
	return Py_None;

    key = PyTuple_New(2);
    if (key == NULL)
	goto onError;
    v = PyInt_FromLong((long) definition);
    if (v == NULL)
	goto onError;
    PyTuple_SET_ITEM(key, 0, v);
    v = PyInt_FromLong(tabletype);
    if (v == NULL)
	goto onError;
    PyTuple_SET_ITEM(key, 1, v);
    tt = PyDict_GetItem(mxTextTools_TagTables, key);
    Py_DECREF(key);
    if (tt != NULL) {
	Py_INCREF(tt);
	return tt;
    }
    return Py_None;

 onError:
    return NULL;
}

/* Adds the compiled tagtable to the cache. Returns -1 in case of an
   error, 0 on success. */

static
int add_to_tagtable_cache(PyObject *definition,
			  int tabletype,
			  int cacheable,
			  PyObject *tagtable)
{
    PyObject *v, *key;
    int rc;

    if (!PyTuple_Check(definition) || !cacheable)
	return 0;
    
    key = PyTuple_New(2);
    if (key == NULL)
	goto onError;
    v = PyInt_FromLong((long) definition);
    if (v == NULL)
	goto onError;
    PyTuple_SET_ITEM(key, 0, v);
    v = PyInt_FromLong(tabletype);
    if (v == NULL)
	goto onError;
    PyTuple_SET_ITEM(key, 1, v);

    /* Hard-limit the cache size */
    if (PyDict_Size(mxTextTools_TagTables) >= MAX_TAGTABLES_CACHE_SIZE)
	PyDict_Clear(mxTextTools_TagTables);

    rc = PyDict_SetItem(mxTextTools_TagTables, key, tagtable);
    Py_DECREF(key);
    if (rc)
	goto onError;
    return 0;

 onError:
    return -1;
}

		       
/* allocation */

PyObject *mxTagTable_New(PyObject *definition,
			 int tabletype,
			 int cacheable)
{
    mxTagTableObject *tagtable = 0;
    PyObject *v;
    Py_ssize_t size;

    /* First, consult the TagTable cache */
    v = consult_tagtable_cache(definition, tabletype, cacheable);
    if (v == NULL)
	goto onError;
    else if (v != Py_None)
	return v;

    size = tc_length(definition);
    if (size < 0)
	Py_Error(PyExc_TypeError,
		 "tag table definition must be a tuple or a list");

    tagtable = PyObject_NEW_VAR(mxTagTableObject, &mxTagTable_Type, size);
    if (tagtable == NULL) 
	goto onError;
    if (cacheable) {
	Py_INCREF(definition);
	tagtable->definition = definition;
    }
    else
	tagtable->definition = NULL;
    tagtable->tabletype = tabletype;
    
    /* Compile table ... */
    if (init_tag_table(tagtable, definition, size, tabletype, cacheable))
	goto onError;

    /* Cache the compiled table if it is cacheable and derived from a
       tuple */
    if (add_to_tagtable_cache(definition, tabletype, cacheable, 
			      (PyObject *)tagtable))
	goto onError;

    return (PyObject *)tagtable;

 onError:
    Py_XDECREF(tagtable);
    return NULL;
}

Py_C_Function( mxTagTable_TagTable,
	       "TagTable(definition[,cachable=1])\n\n"
	       )
{
    PyObject *definition;
    int cacheable = 1;

    Py_Get2Args("O|i:TagTable", definition, cacheable);
    return mxTagTable_New(definition, 0, cacheable);

 onError:
    return NULL;
}

#ifdef HAVE_UNICODE
Py_C_Function( mxTagTable_UnicodeTagTable,
	       "TagTable(definition[,cachable=1])\n\n"
	       )
{
    PyObject *definition;
    int cacheable = 1;

    Py_Get2Args("O|i:UnicodeTagTable", definition, cacheable);
    return mxTagTable_New(definition, 1, cacheable);

 onError:
    return NULL;
}
#endif

static 
void mxTagTable_Free(mxTagTableObject *tagtable)
{
    tc_cleanup(tagtable);
    Py_XDECREF(tagtable->definition);
    PyObject_Del(tagtable);
}

/* C APIs */

#define tagtable ((mxTagTableObject *)self)

static
PyObject *mxTagTable_CompiledDefinition(PyObject *self)
{
    PyObject *tuple = 0, *v, *w;
    Py_ssize_t i;
    Py_ssize_t size;

    if (!mxTagTable_Check(self)) {
	PyErr_BadInternalCall();
	goto onError;
    }

    size = tagtable->ob_size;
    tuple = PyTuple_New(size);
    if (tuple == NULL)
	goto onError;
    
    for (i = 0; i < size; i++) {
	mxTagTableEntry *tagtableentry = &tagtable->entry[i];

	/* Build tuple (tagobj, command, args, jne, je) */
	v = PyTuple_New(5);
	if (v == NULL)
	    goto onError;
	w = tagtableentry->tagobj;
	if (w == NULL)
	    w = Py_None;
	Py_INCREF(w);
	PyTuple_SET_ITEM(v, 0, w);
	PyTuple_SET_ITEM(v, 1, PyInt_FromLong(tagtableentry->cmd |
					      tagtableentry->flags));
	w = tagtableentry->args;
	if (w == NULL)
	    w = Py_None;
	Py_INCREF(w);
	PyTuple_SET_ITEM(v, 2, w);
	PyTuple_SET_ITEM(v, 3, PyInt_FromLong(tagtableentry->jne));
	PyTuple_SET_ITEM(v, 4, PyInt_FromLong(tagtableentry->je));
	if (PyErr_Occurred()) {
	    Py_DECREF(v);
	    goto onError;
	}
	PyTuple_SET_ITEM(tuple, i, v);
    }
    
    return tuple;

 onError:
    Py_XDECREF(tuple);
    return NULL;
}


/* methods */

Py_C_Function( mxTagTable_compiled,
	       ".compiled()\n\n"
	       )
{
    Py_NoArgsCheck();
    return mxTagTable_CompiledDefinition(self);

 onError:
    return NULL;
}

#ifdef COPY_PROTOCOL
Py_C_Function( mxTagTable_copy,
	       "copy([memo])\n\n"
	       "Return a new reference for the instance. This function\n"
	       "is used for the copy-protocol. Real copying doesn't take\n"
	       "place, since the instances are immutable.")
{
    PyObject *memo;
    
    Py_GetArg("|O",memo);
    Py_INCREF(tagtable);
    return (PyObject *)tagtable;

 onError:
    return NULL;
}
#endif

#undef tagtable

/* --- slots --- */

static
PyObject *mxTagTable_Repr(mxTagTableObject *self)
{
    char t[100];

    if (self->tabletype == MXTAGTABLE_STRINGTYPE)
	sprintf(t,"<String Tag Table object at 0x%lx>", (long)self);
    else if (self->tabletype == MXTAGTABLE_UNICODETYPE)
	sprintf(t,"<Unicode Tag Table object at 0x%lx>", (long)self);
    else
	sprintf(t,"<Tag Table object at 0x%lx>", (long)self);
    return PyString_FromString(t);
}

static 
PyObject *mxTagTable_GetAttr(mxTagTableObject *self,
			     char *name)
{
    PyObject *v;
    
    if (Py_WantAttr(name,"definition")) {
	v = self->definition;
	if (v == NULL)
	    v = Py_None;
	Py_INCREF(v);
	return v;
    }
    else if (Py_WantAttr(name,"__members__"))
	return Py_BuildValue("[s]",
			     "definition");
    
    return Py_FindMethod(mxTagTable_Methods, (PyObject *)self, (char *)name);
}

/* Python Type Tables */

PyTypeObject mxTagTable_Type = {
        PyObject_HEAD_INIT(0)		/* init at startup ! */
	0,			  	/* ob_size */
	"Tag Table",		  	/* tp_name */
	sizeof(mxTagTableObject),	/* tp_basicsize */
	sizeof(mxTagTableEntry),  	/* tp_itemsize */
	/* methods */
	(destructor)mxTagTable_Free,	/* tp_dealloc */
	(printfunc)0,			/* tp_print */
	(getattrfunc)mxTagTable_GetAttr, /* tp_getattr */
	(setattrfunc)0,		  	/* tp_setattr */
	(cmpfunc)0,		  	/* tp_compare */
	(reprfunc)mxTagTable_Repr,  	/* tp_repr */
        0,			  	/* tp_as_number */
        0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	(hashfunc)0,			/* tp_hash */
	(ternaryfunc)0,			/* tp_call */
	(reprfunc)0,			/* tp_str */
	(getattrofunc)0, 		/* tp_getattro */
	(setattrofunc)0, 		/* tp_setattro */
        0,				/* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,		/* tp_flags */
        (char*) 0,			/* tp_doc */
};

/* Python Method Table */

statichere
PyMethodDef mxTagTable_Methods[] =
{   
    Py_MethodListEntryNoArgs("compiled",mxTagTable_compiled),
#ifdef COPY_PROTOCOL
    Py_MethodListEntry("__deepcopy__",mxTagTable_copy),
    Py_MethodListEntry("__copy__",mxTagTable_copy),
#endif
    {NULL,NULL} /* end of list */
};

/* --- Internal functions ----------------------------------------------*/

#ifdef HAVE_UNICODE

/* Same as mxTextTools_Join() for Unicode objects. */

static
PyObject *mxTextTools_UnicodeJoin(PyObject *seq,
				  Py_ssize_t start,
				  Py_ssize_t stop,
				  PyObject *separator)
{
    PyObject *newstring = 0, *tempstr = 0;
    Py_ssize_t newstring_len,current_len = 0;
    Py_UNICODE *p;
    Py_ssize_t i;
    Py_UNICODE *sep;
    Py_ssize_t sep_len;
    
    if (separator) {
	separator = PyUnicode_FromObject(separator);
	if (separator == NULL)
	    goto onError;
	sep = PyUnicode_AS_UNICODE(separator);
	sep_len = PyUnicode_GET_SIZE(separator);
    }
    else {
	sep = NULL;
	sep_len = 0;
    }
    
    /* Create an empty new string */
    newstring_len = (10 + sep_len) * (stop - start);
    newstring = PyUnicode_FromUnicode(NULL, newstring_len);
    if (newstring == NULL) 
	goto onError;
    p = PyUnicode_AS_UNICODE(newstring);

    /* Join with separator */
    for (i = start; i < stop; i++) {
	register PyObject *o;
	Py_UNICODE *st;
	Py_ssize_t len_st;

	o = PySequence_GetItem(seq, i);

	if PyTuple_Check(o) {
	    /* Tuple entry: (string,l,r,[...]) */
	    register Py_ssize_t l,r;

	    /* parse tuple */
	    Py_Assert((PyTuple_GET_SIZE(o) >= 3) &&
		      PyInt_Check(PyTuple_GET_ITEM(o,1)) && 
		      PyInt_Check(PyTuple_GET_ITEM(o,2)),
		      PyExc_TypeError,
		      "tuples must be of the format (string,l,r[,...])");
	    tempstr = PyUnicode_FromObject(PyTuple_GET_ITEM(o,0));
	    if (tempstr == NULL)
		goto onError;
	    st = PyUnicode_AS_UNICODE(tempstr);
	    len_st = PyUnicode_GET_SIZE(tempstr);
	    l = PyInt_AS_LONG(PyTuple_GET_ITEM(o,1));
	    r = PyInt_AS_LONG(PyTuple_GET_ITEM(o,2));

	    /* compute slice */
	    if (r > len_st) r = len_st;
	    else if (r < 0) {
		r += len_st + 1;
		if (r < 0)
		    r = 0;
	    }
	    if (l > len_st) l = len_st;
	    else if (l < 0) {
		l += len_st + 1;
		if (l < 0)
		    l = 0;
	    }

	    /* empty ? */
	    if (l > r)
		continue;
	    len_st = r - l;
	    if (len_st == 0) 
		continue;

	    /* get pointer right */
	    st += l;
	}
	else {
	    /* Must be a string entry: take the whole string */
	    tempstr = PyUnicode_FromObject(o);
	    if (tempstr == NULL)
		goto onError;
	    st = PyUnicode_AS_UNICODE(tempstr);
	    len_st = PyUnicode_GET_SIZE(tempstr);
	}

        Py_DECREF(o);
    
	/* Resize the new string if needed */
	while (current_len + len_st + sep_len >= newstring_len) {
	    newstring_len += newstring_len >> 1;
	    if (PyUnicode_Resize(&newstring, newstring_len))
		goto onError;
	    p = PyUnicode_AS_UNICODE(newstring) + current_len;
	}

	/* Insert separator */
	if (i > 0 && sep_len > 0) {
	    Py_UNICODE_COPY(p, sep, sep_len);
	    p += sep_len;
	    current_len += sep_len;
	}

	/* Copy snippet into new string */
	Py_UNICODE_COPY(p, st, len_st);
	p += len_st;
	current_len += len_st;
	
	Py_DECREF(tempstr);
	tempstr = NULL;
    }
    
    /* Resize new string to the actual length */
    if (PyUnicode_Resize(&newstring, current_len))
	goto onError;

    Py_XDECREF(separator);
    return newstring;

 onError:
    Py_XDECREF(newstring);
    Py_XDECREF(separator);
    Py_XDECREF(tempstr);
    return NULL;
}

#endif

/* Enhanced string join: also excepts tuple (text, left, right,...)
   entries which then cause text[left:right] to be used as string
   snippet.

   separator may be NULL; in that case, "" is used as separator.

*/

static
PyObject *mxTextTools_Join(PyObject *seq,
			   Py_ssize_t start,
			   Py_ssize_t stop,
			   PyObject *separator)
{
    PyObject *newstring = 0;
    Py_ssize_t newstring_len, current_len = 0;
    char *p;
    Py_ssize_t i;
    char *sep;
    Py_ssize_t sep_len;

    if (separator) {
#ifdef HAVE_UNICODE
	if (PyUnicode_Check(separator))
	    return mxTextTools_UnicodeJoin(seq, start, stop, separator);
#endif
	Py_Assert(PyString_Check(separator),
		  PyExc_TypeError,
		  "separator must be a string");
	sep = PyString_AS_STRING(separator);
	sep_len = PyString_GET_SIZE(separator);
    }
    else {
	sep = NULL;
	sep_len = 0;
    }
    
    /* Create an empty new string */
    newstring_len = (10 + sep_len) * (stop - start);
    newstring = PyString_FromStringAndSize((char*)NULL, newstring_len);
    if (newstring == NULL) 
	goto onError;
    p = PyString_AS_STRING(newstring);

    /* Join with separator */
    for (i = start; i < stop; i++) {
	register PyObject *o;
	char *st;
	Py_ssize_t len_st;

	o = PySequence_GetItem(seq, i);

	if PyTuple_Check(o) {
	    /* Tuple entry: (string,l,r,[...]) */
	    register Py_ssize_t l,r;

	    /* parse tuple */
	    Py_Assert((PyTuple_GET_SIZE(o) >= 3) &&
		      PyInt_Check(PyTuple_GET_ITEM(o,1)) && 
		      PyInt_Check(PyTuple_GET_ITEM(o,2)),
		      PyExc_TypeError,
		      "tuples must be of the format (string,int,int[,...])");
#ifdef HAVE_UNICODE
	    if (PyUnicode_Check(PyTuple_GET_ITEM(o,0))) {
		/* Redirect to Unicode implementation; all previous work
		   is lost. */
		Py_DECREF(o);
		Py_DECREF(newstring);
		return mxTextTools_UnicodeJoin(seq, start, stop, separator);
	    }
#endif
	    Py_Assert(PyString_Check(PyTuple_GET_ITEM(o,0)),
		      PyExc_TypeError,
		      "tuples must be of the format (string,int,int[,...])");
	    st = PyString_AS_STRING(PyTuple_GET_ITEM(o,0));
	    len_st = PyString_GET_SIZE(PyTuple_GET_ITEM(o,0));
	    l = PyInt_AS_LONG(PyTuple_GET_ITEM(o,1));
	    r = PyInt_AS_LONG(PyTuple_GET_ITEM(o,2));

	    /* compute slice */
	    if (r > len_st) r = len_st;
	    else if (r < 0) {
		r += len_st + 1;
		if (r < 0)
		    r = 0;
	    }
	    if (l > len_st) l = len_st;
	    else if (l < 0) {
		l += len_st + 1;
		if (l < 0)
		    l = 0;
	    }

	    /* empty ? */
	    if (l > r)
		continue;
	    len_st = r - l;
	    if (len_st == 0) 
		continue;

	    /* get pointer right */
	    st += l;
	}
	else if (PyString_Check(o)) {
	    /* String entry: take the whole string */
	    st = PyString_AS_STRING(o);
	    len_st = PyString_GET_SIZE(o);
	}
#ifdef HAVE_UNICODE
	else if (PyUnicode_Check(o)) {
	    /* Redirect to Unicode implementation; all previous work
	       is lost. */
	    Py_DECREF(o);
	    Py_DECREF(newstring);
	    return mxTextTools_UnicodeJoin(seq, start, stop, separator);
	}
#endif
	else {
	    Py_DECREF(o);
	    Py_Error(PyExc_TypeError,
		     "list must contain tuples or strings as entries");
	}

        Py_DECREF(o);
    
	/* Resize the new string if needed */
	while (current_len + len_st + sep_len >= newstring_len) {
	    newstring_len += newstring_len >> 1;
	    if (_PyString_Resize(&newstring, newstring_len))
		goto onError;
	    p = PyString_AS_STRING(newstring) + current_len;
	}

	/* Insert separator */
	if (i > 0 && sep_len > 0) {
	    memcpy(p, sep, sep_len);
	    p += sep_len;
	    current_len += sep_len;
	}

	/* Copy snippet into new string */
	memcpy(p,st,len_st);
	p += len_st;
	current_len += len_st;
    }
    
    /* Resize new string to the actual length */
    if (_PyString_Resize(&newstring, current_len))
	goto onError;

    return newstring;

 onError:
    Py_XDECREF(newstring);
    return NULL;
}

static
PyObject *mxTextTools_HexStringFromString(char *str,
					  Py_ssize_t len) 
{
    PyObject *w = 0;
    Py_ssize_t i;
    char *hex;
    static const char hexdigits[] = "0123456789abcdef";

    /* Convert to HEX */
    w = PyString_FromStringAndSize(NULL,2*len);
    if (!w)
	goto onError;
    hex = PyString_AS_STRING(w);
    for (i = 0; i < len; i ++) {
	unsigned char c = (unsigned char)*str;
	
	*hex++ = hexdigits[c >> 4];
	*hex++ = hexdigits[c & 0x0F];
	str++;
    }
    return w;

 onError:
    Py_XDECREF(w);
    return NULL;
}

static
PyObject *mxTextTools_StringFromHexString(char *hex,
					  Py_ssize_t len)
{
    PyObject *w = 0;
    Py_ssize_t i;
    char *str;
    static const char hexdigits[] = "0123456789abcdef";

    /* Convert to string */
    Py_Assert(len % 2 == 0,
	      PyExc_TypeError,
	      "need 2-digit hex string argument");
    len >>= 1;
    w = PyString_FromStringAndSize(NULL,len);
    if (!w)
	goto onError;
    str = PyString_AS_STRING(w);
    for (i = 0; i < len; i++,str++) {
	register char c;
	register Py_ssize_t j;

	c = tolower(*hex++);
	for (j = 0; j < (Py_ssize_t)sizeof(hexdigits); j++)
	  if (c == hexdigits[j]) {
	    *str = j << 4;
	    break;
	  }
	if (j == sizeof(hexdigits)) {
	  DPRINTF("Failed: '%c' (%u) at %i\n",c,(unsigned int)c,i);
	  Py_Error(PyExc_ValueError,
		   "argument contains non-hex characters");
	}

	c = tolower(*hex++);
	for (j = 0; j < (Py_ssize_t)sizeof(hexdigits); j++)
	  if (c == hexdigits[j]) {
	    *str += j;
	    break;
	  }
	if (j == sizeof(hexdigits)) {
	  DPRINTF("Failed2: '%c' (%u) at %i\n",c,(unsigned int)c,i);
	  Py_Error(PyExc_ValueError,
		   "argument contains non-hex characters");
	}
    }
    return w;

 onError:
    Py_XDECREF(w);
    return NULL;
}

static 
int mxTextTools_IsASCII(PyObject *text,
			Py_ssize_t left,
			Py_ssize_t right)
{
    if (PyString_Check(text)) {
	Py_ssize_t len;
	register Py_ssize_t i;
	register unsigned char *str = (unsigned char *)PyString_AS_STRING(text);

	len = PyString_GET_SIZE(text);
	Py_CheckSequenceSlice(len, left, right);
	for (i = left; i < right; i++)
	    if (str[i] >= 128)
		return 0;
	return 1;
    }

#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_ssize_t len;
	register Py_ssize_t i;
	register Py_UNICODE *str = PyUnicode_AS_UNICODE(text);

	len = PyUnicode_GET_SIZE(text);
	Py_CheckSequenceSlice(len, left, right);
	for (i = left; i < right; i++)
	    if (str[i] >= 128)
		return 0;
	return 1;
    }
#endif

    else
	Py_Error(PyExc_TypeError,
		 "need string object");
    
 onError:
    return -1;
}

/* Takes a list of tuples (replacement,l,r,...) and produces a taglist
   suitable for mxTextTools_Join() which creates a copy of
   text where every slice [l:r] is replaced by the given replacement.

*/

static 
PyObject *mxTextTools_Joinlist(PyObject *text,
			       PyObject *list,
			       Py_ssize_t pos,
			       Py_ssize_t text_len)
{
    PyObject *joinlist = 0;
    Py_ssize_t list_len;
    Py_ssize_t i;
    Py_ssize_t listitem = 0;
    Py_ssize_t listsize = INITIAL_LIST_SIZE;
    
    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, pos, text_len);
    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, pos, text_len);
    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    
    Py_Assert(PyList_Check(list),
	      PyExc_TypeError,
	      "expected a list of tuples as second argument");
    list_len = PyList_GET_SIZE(list);

    joinlist = PyList_New(listsize);
    if (joinlist == NULL)
	goto onError;

    for (i = 0; i < list_len; i++) {
	register PyObject *t;
	register Py_ssize_t left, right;
	
	t = PyList_GET_ITEM(list, i);
	Py_Assert(PyTuple_Check(t) && 
		  (PyTuple_GET_SIZE(t) >= 3) &&
		  (PyString_Check(PyTuple_GET_ITEM(t,0)) ||
		   PyUnicode_Check(PyTuple_GET_ITEM(t,0))) &&
		  PyInt_Check(PyTuple_GET_ITEM(t,1)) &&
		  PyInt_Check(PyTuple_GET_ITEM(t,2)),
		  PyExc_TypeError,
		  "tuples must be of the form (string,int,int,...)");
	left = PyInt_AS_LONG(PyTuple_GET_ITEM(t,1));
	right = PyInt_AS_LONG(PyTuple_GET_ITEM(t,2));

	Py_Assert(left >= pos,
		  PyExc_ValueError,
		  "list is not sorted ascending");

	if (left > pos) { /* joinlist.append((text,pos,left)) */
	    register PyObject *v;
	    register PyObject *w;
	    
	    v = PyTuple_New(3);
	    if (v == NULL)
		goto onError;

	    Py_INCREF(text);
	    PyTuple_SET_ITEM(v,0,text);

	    w = PyInt_FromLong(pos);
	    if (w == NULL)
		goto onError;
	    PyTuple_SET_ITEM(v,1,w);

	    w = PyTuple_GET_ITEM(t,1);
	    Py_INCREF(w);
	    PyTuple_SET_ITEM(v,2,w);

	    if (listitem < listsize)
		PyList_SET_ITEM(joinlist,listitem,v);
	    else {
		PyList_Append(joinlist,v);
		Py_DECREF(v);
	    }
	    listitem++;
	}
	
	/* joinlist.append(string) */
	if (listitem < listsize) {
	    register PyObject *v = PyTuple_GET_ITEM(t,0);
	    Py_INCREF(v);
	    PyList_SET_ITEM(joinlist,listitem,v);
	}
	else
	    PyList_Append(joinlist,PyTuple_GET_ITEM(t,0));
	listitem++;
	
	pos = right;
    }
    
    if (pos < text_len) { /* joinlist.append((text,pos,text_len)) */
	register PyObject *v;
	register PyObject *w;
	    
	v = PyTuple_New(3);
	if (v == NULL)
	    goto onError;

	Py_INCREF(text);
	PyTuple_SET_ITEM(v,0,text);

	w = PyInt_FromLong(pos);
	if (w == NULL)
	    goto onError;
	PyTuple_SET_ITEM(v,1,w);

	w = PyInt_FromLong(text_len);
	if (w == NULL)
	    goto onError;
	PyTuple_SET_ITEM(v,2,w);

	if (listitem < listsize)
	    PyList_SET_ITEM(joinlist,listitem,v);
	else {
	    PyList_Append(joinlist,v);
	    Py_DECREF(v);
	}
	listitem++;
    }

    /* Resize list if necessary */
    if (listitem < listsize)
	PyList_SetSlice(joinlist,listitem,listsize,(PyObject*)NULL);

    return joinlist;

 onError:

    Py_XDECREF(joinlist);
    return NULL;
}

#ifdef HAVE_UNICODE
static 
PyObject *mxTextTools_UnicodeCharSplit(PyObject *text,
				       PyObject *separator,
				       Py_ssize_t start,
				       Py_ssize_t text_len)
{
    PyObject *list = NULL;
    register Py_ssize_t x;
    Py_ssize_t listitem = 0;
    Py_ssize_t listsize = INITIAL_LIST_SIZE;
    Py_UNICODE *tx;
    Py_UNICODE sep;

    text = PyUnicode_FromObject(text);
    if (text == NULL) {
	separator = NULL;
	goto onError;
    }
    separator = PyUnicode_FromObject(separator);
    if (separator == NULL)
	goto onError;

    Py_CheckUnicodeSlice(text, start, text_len);

    Py_Assert(PyUnicode_GET_SIZE(separator) == 1,
	      PyExc_TypeError,
	      "separator must be a single character");

    tx = PyUnicode_AS_UNICODE(text);
    sep = *PyUnicode_AS_UNICODE(separator);

    list = PyList_New(listsize);
    if (!list)
	goto onError;

    x = start;
    while (1) {
	PyObject *s;
	register Py_ssize_t z;

	/* Skip to next separator */
	z = x;
	for (;x < text_len; x++) 
	    if (tx[x] == sep)
		break;

	/* Append the slice to list */
	s = PyUnicode_FromUnicode(&tx[z], x - z);
	if (!s)
	    goto onError;
	if (listitem < listsize)
	    PyList_SET_ITEM(list,listitem,s);
	else {
	    PyList_Append(list,s);
	    Py_DECREF(s);
	}
	listitem++;

	if (x == text_len)
	    break;

	/* Skip separator */
	x++;
    }

    /* Resize list if necessary */
    if (listitem < listsize)
	PyList_SetSlice(list,listitem,listsize,(PyObject*)NULL);

    Py_DECREF(text);
    Py_DECREF(separator);
    return list;
    
 onError:
    Py_XDECREF(list);
    Py_XDECREF(text);
    Py_XDECREF(separator);
    return NULL;
}
#endif

static 
PyObject *mxTextTools_CharSplit(PyObject *text,
				PyObject *separator,
				Py_ssize_t start,
				Py_ssize_t text_len)
{
    PyObject *list = 0;
    register Py_ssize_t x;
    Py_ssize_t listitem = 0;
    Py_ssize_t listsize = INITIAL_LIST_SIZE;
    char *tx;
    char sep;

#ifdef HAVE_UNICODE
    if (PyUnicode_Check(text) || PyUnicode_Check(separator))
	return mxTextTools_UnicodeCharSplit(text, separator, 
					    start, text_len);
#endif

    if (PyString_Check(text) && PyString_Check(separator)) {
	Py_CheckStringSlice(text, start, text_len);
    }
    else
	Py_Error(PyExc_TypeError,
		 "text and separator must be strings or unicode");

    Py_Assert(PyString_GET_SIZE(separator) == 1,
	      PyExc_TypeError,
	      "separator must be a single character");
    
    tx = PyString_AS_STRING(text);
    sep = *PyString_AS_STRING(separator);

    list = PyList_New(listsize);
    if (!list)
	goto onError;

    x = start;
    while (1) {
	PyObject *s;
	register Py_ssize_t z;

	/* Skip to next separator */
	z = x;
	for (;x < text_len; x++) 
	    if (tx[x] == sep)
		break;

	/* Append the slice to list */
	s = PyString_FromStringAndSize(&tx[z], x - z);
	if (!s)
	    goto onError;
	if (listitem < listsize)
	    PyList_SET_ITEM(list,listitem,s);
	else {
	    PyList_Append(list,s);
	    Py_DECREF(s);
	}
	listitem++;

	if (x == text_len)
	    break;

	/* Skip separator */
	x++;
    }

    /* Resize list if necessary */
    if (listitem < listsize)
	PyList_SetSlice(list,listitem,listsize,(PyObject*)NULL);

    return list;
    
 onError:
    Py_XDECREF(list);
    return NULL;
}

#ifdef HAVE_UNICODE
static 
PyObject *mxTextTools_UnicodeSplitAt(PyObject *text,
				     PyObject *separator,
				     Py_ssize_t nth,
				     Py_ssize_t start,
				     Py_ssize_t text_len)
{
    PyObject *tuple = 0;
    register Py_ssize_t x;
    PyObject *s;
    Py_UNICODE *tx;
    Py_UNICODE sep;

    text = PyUnicode_FromObject(text);
    if (text == NULL) {
	separator = NULL;
	goto onError;
    }
    separator = PyUnicode_FromObject(separator);
    if (separator == NULL)
	goto onError;

    Py_CheckUnicodeSlice(text, start, text_len);

    Py_Assert(PyUnicode_GET_SIZE(separator) == 1,
	      PyExc_TypeError,
	      "separator must be a single character");

    tx = PyUnicode_AS_UNICODE(text);
    sep = *PyUnicode_AS_UNICODE(separator);

    tuple = PyTuple_New(2);
    if (!tuple)
	goto onError;

    if (nth > 0) {
	/* Skip to nth separator from the left */
	x = start;
	while (1) {
	    for (; x < text_len; x++) 
		if (tx[x] == sep)
		    break;
	    if (--nth == 0 || x == text_len)
		break;
	    x++;
	}
    }
    else if (nth < 0) {
	/* Skip to nth separator from the right */
	x = text_len - 1;
	while (1) {
	    for (; x >= start; x--) 
		if (tx[x] == sep)
		    break;
	    if (++nth == 0 || x < start)
		break;
	    x--;
	}
    }
    else
	Py_Error(PyExc_ValueError,
		 "nth must be non-zero");
    
    /* Add to tuple */
    if (x < start)
	s = PyUnicode_FromUnicode((Py_UNICODE *)"", 0);
    else
	s = PyUnicode_FromUnicode(&tx[start], x - start);
    if (!s)
	goto onError;
    PyTuple_SET_ITEM(tuple,0,s);

    /* Skip separator */
    x++;

    if (x >= text_len)
	s = PyUnicode_FromUnicode((Py_UNICODE *)"", 0);
    else
	s = PyUnicode_FromUnicode(&tx[x], text_len - x);
    if (!s)
	goto onError;
    PyTuple_SET_ITEM(tuple,1,s);

    Py_DECREF(text);
    Py_DECREF(separator);
    return tuple;
    
 onError:
    Py_XDECREF(tuple);
    Py_XDECREF(text);
    Py_XDECREF(separator);
    return NULL;
}
#endif

static 
PyObject *mxTextTools_SplitAt(PyObject *text,
			      PyObject *separator,
			      Py_ssize_t nth,
			      Py_ssize_t start,
			      Py_ssize_t text_len)
{
    PyObject *tuple = 0;
    register Py_ssize_t x;
    PyObject *s;
    char *tx;
    char sep;

#ifdef HAVE_UNICODE
    if (PyUnicode_Check(text) || PyUnicode_Check(separator))
	return mxTextTools_UnicodeSplitAt(text, separator, 
					  nth, start, text_len);
#endif

    if (PyString_Check(text) && PyString_Check(separator)) {
	Py_CheckStringSlice(text, start, text_len);
    }
    else
	Py_Error(PyExc_TypeError,
		 "text and separator must be strings or unicode");

    Py_Assert(PyString_GET_SIZE(separator) == 1,
	      PyExc_TypeError,
	      "separator must be a single character");

    tx = PyString_AS_STRING(text);
    sep = *PyString_AS_STRING(separator);

    tuple = PyTuple_New(2);
    if (!tuple)
	goto onError;

    if (nth > 0) {
	/* Skip to nth separator from the left */
	x = start;
	while (1) {
	    for (; x < text_len; x++) 
		if (tx[x] == sep)
		    break;
	    if (--nth == 0 || x == text_len)
		break;
	    x++;
	}
    }
    else if (nth < 0) {
	/* Skip to nth separator from the right */
	x = text_len - 1;
	while (1) {
	    for (; x >= start; x--) 
		if (tx[x] == sep)
		    break;
	    if (++nth == 0 || x < start)
		break;
	    x--;
	}
    }
    else
	Py_Error(PyExc_ValueError,
		 "nth must be non-zero");
    
    /* Add to tuple */
    if (x < start)
	s = PyString_FromStringAndSize("",0);
    else
	s = PyString_FromStringAndSize(&tx[start], x - start);
    if (!s)
	goto onError;
    PyTuple_SET_ITEM(tuple,0,s);

    /* Skip separator */
    x++;

    if (x >= text_len)
	s = PyString_FromStringAndSize("",0);
    else
	s = PyString_FromStringAndSize(&tx[x], text_len - x);
    if (!s)
	goto onError;
    PyTuple_SET_ITEM(tuple,1,s);

    return tuple;
    
 onError:
    Py_XDECREF(tuple);
    return NULL;
}

#ifdef HAVE_UNICODE
static 
PyObject *mxTextTools_UnicodeSuffix(PyObject *text,
				    PyObject *suffixes,
				    Py_ssize_t start,
				    Py_ssize_t text_len,
				    PyObject *translate)
{
    Py_ssize_t i;
    Py_UNICODE *tx;

    text = PyUnicode_FromObject(text);
    if (text == NULL)
	goto onError;
    
    if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, start, text_len);
    }
    else
	Py_Error(PyExc_TypeError,
		 "expected unicode");
    Py_Assert(PyTuple_Check(suffixes),
	      PyExc_TypeError,
	      "suffixes needs to be a tuple of unicode strings");

    /* XXX Add support for translate... */
    Py_Assert(translate == NULL,
	      PyExc_TypeError,
	      "translate is not supported for Unicode suffix()es");

    tx = PyUnicode_AS_UNICODE(text);

    for (i = 0; i < PyTuple_GET_SIZE(suffixes); i++) {
	PyObject *suffix = PyTuple_GET_ITEM(suffixes,i);
	Py_ssize_t start_cmp;

	suffix = PyUnicode_FromObject(suffix);
	if (suffix == NULL) 
	    goto onError;

	start_cmp = text_len - PyUnicode_GET_SIZE(suffix);
	if (start_cmp >= start &&
	    PyUnicode_AS_UNICODE(suffix)[0] == tx[start_cmp] &&
	    memcmp(PyUnicode_AS_UNICODE(suffix),
		   &tx[start_cmp],
		   PyUnicode_GET_DATA_SIZE(suffix)) == 0) {
	    Py_DECREF(text);
	    return suffix;
	}

	Py_DECREF(suffix);
    }

    Py_DECREF(text);
    Py_ReturnNone();

 onError:
    Py_XDECREF(text);
    return NULL;
}
#endif

static 
PyObject *mxTextTools_Suffix(PyObject *text,
			     PyObject *suffixes,
			     Py_ssize_t start,
			     Py_ssize_t text_len,
			     PyObject *translate)
{
    Py_ssize_t i;
    char *tx;

#ifdef HAVE_UNICODE
    if (PyUnicode_Check(text))
	return mxTextTools_UnicodeSuffix(text, suffixes, 
					 start, text_len,
					 translate);
#endif

    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, text_len);
    }
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    Py_Assert(PyTuple_Check(suffixes),
	      PyExc_TypeError,
	      "suffixes needs to be a tuple of strings");
    tx = PyString_AS_STRING(text);

    if (translate) {
	char *tr;

	Py_Assert(PyString_Check(translate) && 
		  PyString_GET_SIZE(translate) == 256,
		  PyExc_TypeError,
		  "translate must be a string having 256 characters");
	tr = PyString_AS_STRING(translate);

	for (i = 0; i < PyTuple_GET_SIZE(suffixes); i++) {
	    PyObject *suffix = PyTuple_GET_ITEM(suffixes, i);
	    Py_ssize_t start_cmp;
	    register char *s;
	    register char *t;
	    register Py_ssize_t j;

	    Py_AssertWithArg(PyString_Check(suffix),
			     PyExc_TypeError,
			     "tuple entry %d is not a string",(unsigned int)i);
	    start_cmp = text_len - PyString_GET_SIZE(suffix);
	    if (start_cmp < start)
		continue;

	    /* Do the compare using a translate table */
	    s = PyString_AS_STRING(suffix);
	    t = tx + start_cmp;
	    for (j = start_cmp; j < text_len; j++, s++, t++)
		if (*s != tr[(unsigned char)*t])
		    break;
	    if (j == text_len) {
		Py_INCREF(suffix);
		return suffix;
	    }
	}
    }

    else
	for (i = 0; i < PyTuple_GET_SIZE(suffixes); i++) {
	    PyObject *suffix = PyTuple_GET_ITEM(suffixes,i);
	    Py_ssize_t start_cmp;

	    Py_AssertWithArg(PyString_Check(suffix),
			     PyExc_TypeError,
			     "tuple entry %d is not a string",(unsigned int)i);
	    start_cmp = text_len - PyString_GET_SIZE(suffix);
	    if (start_cmp < start)
		continue;

	    /* Compare without translate table */
	    if (PyString_AS_STRING(suffix)[0] == tx[start_cmp]
		&&
		strncmp(PyString_AS_STRING(suffix),
			&tx[start_cmp],
			PyString_GET_SIZE(suffix)) == 0) {
		Py_INCREF(suffix);
		return suffix;
	    }
	}

    Py_ReturnNone();
    
 onError:
    return NULL;
}

#ifdef HAVE_UNICODE
static 
PyObject *mxTextTools_UnicodePrefix(PyObject *text,
				    PyObject *prefixes,
				    Py_ssize_t start,
				    Py_ssize_t text_len,
				    PyObject *translate)
{
    Py_ssize_t i;
    Py_UNICODE *tx;

    text = PyUnicode_FromObject(text);
    if (text == NULL)
	goto onError;
    
    if (PyUnicode_Check(text)) {
	Py_CheckUnicodeSlice(text, start, text_len);
    }
    else
	Py_Error(PyExc_TypeError,
		 "expected unicode");
    Py_Assert(PyTuple_Check(prefixes),
	      PyExc_TypeError,
	      "prefixes needs to be a tuple of unicode strings");

    /* XXX Add support for translate... */
    Py_Assert(translate == NULL,
	      PyExc_TypeError,
	      "translate is not supported for Unicode prefix()es");

    tx = PyUnicode_AS_UNICODE(text);

    for (i = 0; i < PyTuple_GET_SIZE(prefixes); i++) {
	PyObject *prefix = PyTuple_GET_ITEM(prefixes,i);

	prefix = PyUnicode_FromObject(prefix);
	if (prefix == NULL) 
	    goto onError;

	/* Compare without translate table */
	if (start + PyString_GET_SIZE(prefix) <= text_len &&
	    PyUnicode_AS_UNICODE(prefix)[0] == tx[start] &&
	    memcmp(PyUnicode_AS_UNICODE(prefix),
		   &tx[start],
		   PyUnicode_GET_DATA_SIZE(prefix)) == 0) {
	    Py_INCREF(prefix);
	    return prefix;
	}

	Py_DECREF(prefix);
    }

    Py_DECREF(text);
    Py_ReturnNone();

 onError:
    Py_XDECREF(text);
    return NULL;
}
#endif

static 
PyObject *mxTextTools_Prefix(PyObject *text,
			     PyObject *prefixes,
			     Py_ssize_t start,
			     Py_ssize_t text_len,
			     PyObject *translate)
{
    Py_ssize_t i;
    char *tx;

#ifdef HAVE_UNICODE
    if (PyUnicode_Check(text))
	return mxTextTools_UnicodePrefix(text, prefixes, 
					 start, text_len,
					 translate);
#endif

    if (PyString_Check(text)) {
	Py_CheckStringSlice(text, start, text_len);
    }
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    Py_Assert(PyTuple_Check(prefixes),
	      PyExc_TypeError,
	      "prefixes needs to be a tuple of strings");
    tx = PyString_AS_STRING(text);

    if (translate) {
	char *tr;

	Py_Assert(PyString_Check(translate) && 
		  PyString_GET_SIZE(translate) == 256,
		  PyExc_TypeError,
		  "translate must be a string having 256 characters");
	tr = PyString_AS_STRING(translate);

	for (i = 0; i < PyTuple_GET_SIZE(prefixes); i++) {
	    PyObject *prefix = PyTuple_GET_ITEM(prefixes,i);
	    Py_ssize_t cmp_len;
	    register char *s;
	    register char *t;
	    register Py_ssize_t j;

	    Py_AssertWithArg(PyString_Check(prefix),
			     PyExc_TypeError,
			     "tuple entry %d is not a string",(unsigned int)i);
	    cmp_len = PyString_GET_SIZE(prefix);
	    if (start + cmp_len > text_len)
		continue;

	    /* Do the compare using a translate table */
	    s = PyString_AS_STRING(prefix);
	    t = tx + start;
	    for (j = 0; j < cmp_len; j++, s++, t++)
		if (*s != tr[(unsigned char)*t])
		    break;
	    if (j == cmp_len) {
		Py_INCREF(prefix);
		return prefix;
	    }
	}
    }

    else
	for (i = 0; i < PyTuple_GET_SIZE(prefixes); i++) {
	    PyObject *prefix = PyTuple_GET_ITEM(prefixes,i);

	    Py_AssertWithArg(PyString_Check(prefix),
			     PyExc_TypeError,
			     "tuple entry %d is not a string",(unsigned int)i);
	    if (start + PyString_GET_SIZE(prefix) > text_len)
		continue;

	    /* Compare without translate table */
	    if (PyString_AS_STRING(prefix)[0] == tx[start] &&
		strncmp(PyString_AS_STRING(prefix),
			&tx[start],
			PyString_GET_SIZE(prefix)) == 0) {
		Py_INCREF(prefix);
		return prefix;
	    }
	}

    Py_ReturnNone();
    
 onError:
    return NULL;
}

/* Stips off characters appearing in the character set from text[start:stop]
   and returns the result as Python string object.

   where indicates the mode:
   where < 0: strip left only
   where = 0: strip left and right
   where > 0: strip right only

*/
static
PyObject *mxTextTools_SetStrip(char *tx,
			       Py_ssize_t tx_len,
			       char *setstr,
			       Py_ssize_t setstr_len,
			       Py_ssize_t start,
			       Py_ssize_t stop,
			       Py_ssize_t where)
{
    Py_ssize_t left, right;

    Py_Assert(setstr_len == 32,
	      PyExc_TypeError,
	      "separator needs to be a set as obtained from set()");
    Py_CheckBufferSlice(tx_len, start, stop);

    /* Strip left */
    if (where <= 0) {
	register Py_ssize_t x;
	for (x = start; x < stop; x++) 
	    if (!Py_CharInSet(tx[x], setstr))
		break;
	left = x;
    }
    else
	left = start;

    /* Strip right */
    if (where >= 0) {
	register Py_ssize_t x;
	for (x = stop - 1; x >= start; x--) 
	    if (!Py_CharInSet(tx[x], setstr))
		break;
	right = x + 1;
    }
    else
	right = stop;
    
    return PyString_FromStringAndSize(tx + left, max(right - left, 0));

 onError:
    return NULL;
}

static 
PyObject *mxTextTools_SetSplit(char *tx,
			       Py_ssize_t tx_len,
			       char *setstr,
			       Py_ssize_t setstr_len,
			       Py_ssize_t start,
			       Py_ssize_t text_len)
{
    PyObject *list = NULL;
    register Py_ssize_t x;
    Py_ssize_t listitem = 0;
    Py_ssize_t listsize = INITIAL_LIST_SIZE;

    Py_Assert(setstr_len == 32,
	      PyExc_TypeError,
	      "separator needs to be a set as obtained from set()");
    Py_CheckBufferSlice(tx_len,start,text_len);

    list = PyList_New(listsize);
    if (!list)
	goto onError;

    x = start;
    while (x < text_len) {
	Py_ssize_t z;

	/* Skip all text in set */
	for (;x < text_len; x++) {
	    register Py_ssize_t c = (unsigned char)tx[x];
	    register Py_ssize_t block = (unsigned char)setstr[c >> 3];
	    if (!block || ((block & (1 << (c & 7))) == 0))
		break;
	}

	/* Skip all text not in set */
	z = x;
	for (;x < text_len; x++) {
	    register Py_ssize_t c = (unsigned char)tx[x];
	    register Py_ssize_t block = (unsigned char)setstr[c >> 3];
	    if (block && ((block & (1 << (c & 7))) != 0))
		break;
	}

	/* Append the slice to list if it is not empty */
	if (x > z) {
	    PyObject *s;
	    s = PyString_FromStringAndSize((char *)&tx[z], x - z);
	    if (!s)
		goto onError;
	    if (listitem < listsize)
		PyList_SET_ITEM(list,listitem,s);
	    else {
		PyList_Append(list,s);
		Py_DECREF(s);
	    }
	    listitem++;
	}
    }

    /* Resize list if necessary */
    if (listitem < listsize)
	PyList_SetSlice(list,listitem,listsize,(PyObject*)NULL);

    return list;
    
 onError:
    Py_XDECREF(list);
    return NULL;
}

static 
PyObject *mxTextTools_SetSplitX(char *tx,
				Py_ssize_t tx_len,
				char *setstr,
				Py_ssize_t setstr_len,
				Py_ssize_t start,
				Py_ssize_t text_len)
{
    PyObject *list = NULL;
    register Py_ssize_t x;
    Py_ssize_t listitem = 0;
    Py_ssize_t listsize = INITIAL_LIST_SIZE;

    Py_Assert(setstr_len == 32,
	      PyExc_TypeError,
	      "separator needs to be a set as obtained from set()");
    Py_CheckBufferSlice(tx_len,start,text_len);

    list = PyList_New(listsize);
    if (!list)
	goto onError;

    x = start;
    while (x < text_len) {
	PyObject *s;
	register Py_ssize_t z;

	/* Skip all text not in set */
	z = x;
	for (;x < text_len; x++) {
	    register unsigned int c = (unsigned char)tx[x];
	    register unsigned int block = (unsigned char)setstr[c >> 3];
	    if (block && ((block & (1 << (c & 7))) != 0))
		break;
	}

	/* Append the slice to list */
	s = PyString_FromStringAndSize((char *)&tx[z], x - z);
	if (!s)
	    goto onError;
	if (listitem < listsize)
	    PyList_SET_ITEM(list,listitem,s);
	else {
	    PyList_Append(list,s);
	    Py_DECREF(s);
	}
	listitem++;

	if (x >= text_len)
	    break;

	/* Skip all text in set */
	z = x;
	for (;x < text_len; x++) {
	    register unsigned int c = (unsigned char)tx[x];
	    register unsigned int block = (unsigned char)setstr[c >> 3];
	    if (!block || ((block & (1 << (c & 7))) == 0))
		break;
	}

	/* Append the slice to list if it is not empty */
	s = PyString_FromStringAndSize((char *)&tx[z], x - z);
	if (!s)
	    goto onError;
	if (listitem < listsize)
	    PyList_SET_ITEM(list,listitem,s);
	else {
	    PyList_Append(list,s);
	    Py_DECREF(s);
	}
	listitem++;
    }

    /* Resize list if necessary */
    if (listitem < listsize)
	PyList_SetSlice(list,listitem,listsize,(PyObject*)NULL);

    return list;
    
 onError:
    Py_XDECREF(list);
    return NULL;
}

static 
PyObject *mxTextTools_Upper(PyObject *text)
{
    PyObject *ntext;
    register unsigned char *s;
    register unsigned char *orig;
    register Py_ssize_t i;
    unsigned char *tr;
    Py_ssize_t len;
    
    Py_Assert(PyString_Check(text),
	      PyExc_TypeError,
	      "expected a Python string");

    len = PyString_GET_SIZE(text);
    ntext = PyString_FromStringAndSize(NULL,len);
    if (!ntext)
	goto onError;
    
    /* Translate */
    tr = (unsigned char *)PyString_AS_STRING(mx_ToUpper);
    orig = (unsigned char *)PyString_AS_STRING(text);
    s = (unsigned char *)PyString_AS_STRING(ntext);
    for (i = 0; i < len; i++, s++, orig++)
	*s = tr[*orig];
    
    return ntext;
    
 onError:
    return NULL;
}

#ifdef HAVE_UNICODE
static 
PyObject *mxTextTools_UnicodeUpper(PyObject *text)
{
    PyObject *ntext;
    register Py_UNICODE *s;
    register Py_UNICODE *orig;
    register Py_ssize_t i;
    Py_ssize_t	len;
    
    text = PyUnicode_FromObject(text);
    if (text == NULL)
	goto onError;

    len = PyUnicode_GET_SIZE(text);
    ntext = PyUnicode_FromUnicode(NULL, len);
    if (!ntext)
	goto onError;
    
    /* Translate */
    orig = (Py_UNICODE *)PyUnicode_AS_UNICODE(text);
    s = (Py_UNICODE *)PyUnicode_AS_UNICODE(ntext);
    for (i = 0; i < len; i++, s++, orig++)
	*s = Py_UNICODE_TOUPPER(*orig);
    
    Py_DECREF(text);
    return ntext;
    
 onError:
    Py_XDECREF(text);
    return NULL;
}
#endif

static 
PyObject *mxTextTools_Lower(PyObject *text)
{
    PyObject *ntext;
    register unsigned char *s;
    register unsigned char *orig;
    register Py_ssize_t i;
    unsigned char *tr;
    Py_ssize_t len;
    
    Py_Assert(PyString_Check(text),
	      PyExc_TypeError,
	      "expected a Python string");

    len = PyString_GET_SIZE(text);
    ntext = PyString_FromStringAndSize(NULL,len);
    if (!ntext)
	goto onError;
    
    /* Translate */
    tr = (unsigned char *)PyString_AS_STRING(mx_ToLower);
    orig = (unsigned char *)PyString_AS_STRING(text);
    s = (unsigned char *)PyString_AS_STRING(ntext);
    for (i = 0; i < len; i++, s++, orig++)
	*s = tr[*orig];
    
    return ntext;
    
 onError:
    return NULL;
}

#ifdef HAVE_UNICODE
static 
PyObject *mxTextTools_UnicodeLower(PyObject *text)
{
    PyObject *ntext;
    register Py_UNICODE *s;
    register Py_UNICODE *orig;
    register Py_ssize_t i;
    Py_ssize_t	len;
    
    text = PyUnicode_FromObject(text);
    if (text == NULL)
	goto onError;

    len = PyUnicode_GET_SIZE(text);
    ntext = PyUnicode_FromUnicode(NULL, len);
    if (!ntext)
	goto onError;
    
    /* Translate */
    orig = (Py_UNICODE *)PyUnicode_AS_UNICODE(text);
    s = (Py_UNICODE *)PyUnicode_AS_UNICODE(ntext);
    for (i = 0; i < len; i++, s++, orig++)
	*s = Py_UNICODE_TOLOWER(*orig);
    
    Py_DECREF(text);
    return ntext;
    
 onError:
    Py_XDECREF(text);
    return NULL;
}
#endif

/* --- Module functions ------------------------------------------------*/

/* Interface to the tagging engine in mxte.c */

Py_C_Function_WithKeywords( 
               mxTextTools_tag,
	       "tag(text,tagtable,sliceleft=0,sliceright=len(text),taglist=[],context=None) \n"""
	       "Produce a tag list for a string, given a tag-table\n"
	       "- returns a tuple (success, taglist, nextindex)\n"
	       "- if taglist == None, then no taglist is created"
	       )
{
    PyObject *text;
    PyObject *tagtable;
    Py_ssize_t sliceright = INT_MAX;
    Py_ssize_t sliceleft = 0;
    PyObject *taglist = 0;
    Py_ssize_t taglist_len;
    PyObject *context = 0;
    Py_ssize_t next, result;
    PyObject *res;
    
    Py_KeywordsGet6Args("OO|iiOO:tag",
			text,tagtable,sliceleft,sliceright,taglist,context);

    if (taglist == NULL) { 
	/* not given, so use default: an empty list */
	taglist = PyList_New(0);
	if (taglist == NULL)
	    goto onError;
	taglist_len = 0;
    }
    else {
	Py_INCREF(taglist);
	Py_Assert(PyList_Check(taglist) || taglist == Py_None,
		  PyExc_TypeError,
		  "taglist must be a list or None");
	if (taglist != Py_None) {
	    taglist_len = PyList_Size(taglist);
	    if (taglist_len < 0)
		goto onError;
	}
	else
	    taglist_len = 0;
    }
    
    Py_Assert(mxTagTable_Check(tagtable) ||
	      PyTuple_Check(tagtable) ||
	      PyList_Check(tagtable),
	      PyExc_TypeError,
	      "tagtable must be a TagTable instance, list or tuple");

    /* Prepare the argument for the Tagging Engine and let it process
       the request */
    if (PyString_Check(text)) {

	Py_CheckStringSlice(text, sliceleft, sliceright);

        if (!mxTagTable_Check(tagtable)) {
	    tagtable = mxTagTable_New(tagtable, MXTAGTABLE_STRINGTYPE, 1);
	    if (tagtable == NULL)
		goto onError;
	}
	else if (mxTagTable_Type(tagtable) != MXTAGTABLE_STRINGTYPE) {
	    Py_Error(PyExc_TypeError,
		     "TagTable instance is not intended for parsing strings");
	}
	else
	    Py_INCREF(tagtable);

	/* Call the Tagging Engine */
	result = mxTextTools_TaggingEngine(text,
					   sliceleft,
					   sliceright,
					   (mxTagTableObject *)tagtable,
					   taglist,
					   context,
					   &next);
	Py_DECREF(tagtable);

    }
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text)) {

	Py_CheckUnicodeSlice(text, sliceleft, sliceright);

        if (!mxTagTable_Check(tagtable)) {
	    tagtable = mxTagTable_New(tagtable, 1, 1);
	    if (tagtable == NULL)
		goto onError;
	}
	else if (mxTagTable_Type(tagtable) != MXTAGTABLE_UNICODETYPE) {
	    Py_Error(PyExc_TypeError,
		     "TagTable instance is not intended for parsing Unicode");
	}
	else
	    Py_INCREF(tagtable);

	/* Call the Tagging Engine */
	result = mxTextTools_UnicodeTaggingEngine(text,
						  sliceleft,
						  sliceright,
						  (mxTagTableObject *)tagtable,
						  taglist,
						  context,
						  &next);
	Py_DECREF(tagtable);

    }
#endif
    else
	Py_Error(PyExc_TypeError,
		 "text must be a string or unicode");

    /* Check for exceptions during matching */
    if (result == 0)
	goto onError;

    /* Undo changes to taglist in case of a match failure (result == 1) */
    if (result == 1 && taglist != Py_None) {
	DPRINTF("  undoing changes: del taglist[%i:%i]\n",
		taglist_len, PyList_Size(taglist));
	if (PyList_SetSlice(taglist, 
			    taglist_len, 
			    PyList_Size(taglist), 
			    NULL))
	    goto onError;
    }

    /* Convert result to the documented external values:
       0 - no match, 1 - match. */
    result--;

    /* Build result tuple */
    res = PyTuple_New(3);
    if (!res)
	goto onError;
    PyTuple_SET_ITEM(res,0,PyInt_FromLong(result));
    PyTuple_SET_ITEM(res,1,taglist);
    PyTuple_SET_ITEM(res,2,PyInt_FromLong(next));
    return res;

 onError:
    if (!PyErr_Occurred())
	Py_Error(PyExc_SystemError,
		 "NULL result without error in builtin tag()");
    Py_XDECREF(taglist);
    return NULL;
}

/* An extended version of string.join() for taglists: */

Py_C_Function( mxTextTools_join,
	       "join(joinlist,sep='',start=0,stop=len(joinlist))\n\n"
	       "Copy snippets from different strings together producing a\n"
	       "new string\n"
	       "The first argument must be a list of tuples or strings;\n"
	       "tuples must be of the form (string,l,r[,...]) and turn out\n"
	       "as string[l:r]\n"
	       "NOTE: the syntax used for negative slices is different\n"
	       "than the Python standard: -1 corresponds to the first\n"
	       "character *after* the string, e.g. ('Example',0,-1) gives\n"
	       "'Example' and not 'Exampl', like in Python\n"
	       "sep is an optional separator string, start and stop\n"
	       "define the slice of joinlist that is taken into accont."
	       )
{
    PyObject *joinlist = NULL;
    Py_ssize_t joinlist_len;
    PyObject *separator = NULL;
    Py_ssize_t start=0, stop=INT_MAX;

    Py_Get4Args("O|Oii:join",
		joinlist,separator,start,stop);

    Py_Assert(PySequence_Check(joinlist),
	      PyExc_TypeError,
	      "first argument needs to be a sequence");

    joinlist_len = PySequence_Length(joinlist);
    Py_Assert(joinlist_len >= 0,
	      PyExc_TypeError,
	      "first argument needs to have a __len__ method");
    
    Py_CheckSequenceSlice(joinlist_len, start, stop);

    /* Short-cut */
    if ((stop - start) <= 0)
	return PyString_FromString("");

    return mxTextTools_Join(joinlist,
			    start, stop,
			    separator);

 onError:
    return NULL;
}

/*
   Special compare function for taglist-tuples, comparing
   the text-slices given:
    - slices starting at a smaller index come first
    - for slices starting at the same index, the longer one
      wins
*/

Py_C_Function( mxTextTools_cmp,
	       "cmp(a,b)\n\n"
	       "Compare two valid taglist tuples w/r to their slice\n"
	       "position; this is useful for sorting joinlists.")
{
    PyObject *v,*w;
    int cmp;

    Py_Get2Args("OO:cmp",v,w);

    Py_Assert(PyTuple_Check(v) && PyTuple_Check(w) && 
	      PyTuple_GET_SIZE(v) >= 3 && PyTuple_GET_SIZE(w) >= 3,
	      PyExc_TypeError,
	      "invalid taglist-tuple");

    cmp = PyObject_Compare(PyTuple_GET_ITEM(v,1),PyTuple_GET_ITEM(w,1));
    if (cmp != 0) 
	return PyInt_FromLong(cmp);
    cmp = - PyObject_Compare(PyTuple_GET_ITEM(v,2),PyTuple_GET_ITEM(w,2));
    return PyInt_FromLong(cmp);

 onError:
    return NULL;
}

Py_C_Function( mxTextTools_joinlist,
	       "joinlist(text,list,start=0,stop=len(text))\n\n"
	       "Takes a list of tuples (replacement,l,r,...) and produces\n"
	       "a taglist suitable for join() which creates a copy\n"
	       "of text where every slice [l:r] is replaced by the\n"
	       "given replacement\n"
	       "- the list must be sorted using cmp() as compare function\n"
	       "- it may not contain overlapping slices\n"
	       "- the slices may not contain negative indices\n"
	       "- if the taglist cannot contain overlapping slices, you can\n"
	       "  give this function the taglist produced by tag() directly\n"
	       "  (sorting is not needed, as the list will already be sorted)\n"
	       "- start and stop set the slice to work in, i.e. text[start:stop]"
)
{
    PyObject *list;
    PyObject *text;
    Py_ssize_t text_len = INT_MAX;
    Py_ssize_t pos = 0;
    
    Py_Get4Args("OO|ii:joinlist",text,list,pos,text_len);

    return mxTextTools_Joinlist(text, list, pos, text_len);

 onError:
    return NULL;
}

Py_C_Function( mxTextTools_charsplit,
	       "charsplit(text,char,start=0,stop=len(text))\n\n"
	       "Split text[start:stop] into substrings at char and\n"
	       "return the result as list of strings."
)
{
    PyObject *text, *separator;
    Py_ssize_t text_len = INT_MAX;
    Py_ssize_t start = 0;

    Py_Get4Args("OO|ii:charsplit",
		text,separator,start,text_len);

    return mxTextTools_CharSplit(text, separator,
				 start, text_len);

 onError:
    return NULL;
}

Py_C_Function( mxTextTools_splitat,
	       "splitat(text,char,nth=1,start=0,stop=len(text))\n\n"
	       "Split text[start:stop] into two substrings at the nth\n"
	       "occurance of char and return the result as 2-tuple. If the\n"
	       "character is not found, the second string is empty. nth may\n"
	       "be negative: the search is then done from the right and the\n"
	       "first string is empty in case the character is not found."
)
{
    PyObject *text, *separator;
    Py_ssize_t text_len = INT_MAX;
    Py_ssize_t start = 0;
    Py_ssize_t nth = 1;

    Py_Get5Args("OO|iii:splitat",
		text,separator,nth,start,text_len);

    return mxTextTools_SplitAt(text, separator,
			       nth, start, text_len);
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_suffix,
	       "suffix(text,suffixes,start=0,stop=len(text)[,translate])\n\n"
	       "Looks at text[start:stop] and returns the first matching\n"
	       "suffix out of the tuple of strings given in suffixes.\n"
	       "If no suffix is found to be matching, None is returned.\n"
	       "The optional 256 char translate string is used to translate\n"
	       "the text prior to comparing it with the given suffixes."
	       )
{
    PyObject *text, *suffixes, *translate = NULL;
    Py_ssize_t text_len = INT_MAX;
    Py_ssize_t start = 0;

    Py_Get5Args("OO|iiO:suffix",
		text,suffixes,start,text_len,translate);

    return mxTextTools_Suffix(text,
			      suffixes,
			      start, text_len,
			      translate);
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_prefix,
	       "prefix(text,prefixes,start=0,stop=len(text)[,translate])\n\n"
	       "Looks at text[start:stop] and returns the first matching\n"
	       "prefix out of the tuple of strings given in prefixes.\n"
	       "If no prefix is found to be matching, None is returned.\n"
	       "The optional 256 char translate string is used to translate\n"
	       "the text prior to comparing it with the given suffixes."
)
{
    PyObject *text, *prefixes, *translate = NULL;
    Py_ssize_t text_len = INT_MAX;
    Py_ssize_t start = 0;

    Py_Get5Args("OO|iiO:prefix",
		text,prefixes,start,text_len,translate);

    return mxTextTools_Prefix(text,
			      prefixes,
			      start, text_len,
			      translate);
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_set,
	       "set(string,logic=1)\n\n"
	       "Returns a character set for string: a bit encoded version\n"
	       "of the characters occurring in string.\n"
	       "- logic can be set to 0 if all characters *not* in string\n"
	       "  should go into the set")
{
    PyObject *sto;
    char *s,*st;
    Py_ssize_t len_s;
    int logic = 1;
    Py_ssize_t i;

    Py_Get3Args("s#|i:set",
		s,len_s,logic);

    sto = PyString_FromStringAndSize(NULL,32);
    if (sto == NULL)
	goto onError;
    
    st = PyString_AS_STRING(sto);

    if (logic) {
	memset(st,0x00,32);
	for (i = 0; i < len_s; i++,s++) {
	    int j = (unsigned char)*s;
	    
	    st[j >> 3] |= 1 << (j & 7);
	}
    }
    else {
	memset(st,0xFF,32);
	for (i = 0; i < len_s; i++,s++) {
	    int j = (unsigned char)*s;
	    
	    st[j >> 3] &= ~(1 << (j & 7));
	}
    }
    return sto;

 onError:
    return NULL;
}

Py_C_Function( mxTextTools_setfind,
	       "setfind(text,set,start=0,stop=len(text))\n\n"
	       "Find the first occurence of any character from set in\n"
	       "text[start:stop]\n set must be a string obtained with set()\n"
	       "DEPRECATED: use CharSet().search() instead."
)
{
    PyObject *text;
    PyObject *set;
    Py_ssize_t text_len = INT_MAX;
    Py_ssize_t start = 0;
    register Py_ssize_t x;
    register char *tx;
    register unsigned char *setstr;
    
    Py_Get4Args("OO|ii:setfind",text,set,start,text_len);

    Py_Assert(PyString_Check(text),
	      PyExc_TypeError,
	      "first argument needs to be a string");
    Py_Assert(PyString_Check(set) && PyString_GET_SIZE(set) == 32,
	      PyExc_TypeError,
	      "second argument needs to be a set");
    Py_CheckStringSlice(text,start,text_len);

    x = start;
    tx = PyString_AS_STRING(text) + x;
    setstr = (unsigned char *)PyString_AS_STRING(set);

    for (;x < text_len; tx++, x++) 
	if (Py_CharInSet(*tx,setstr))
	    break;
    
    if (x == text_len)
	/* Not found */
	return PyInt_FromLong(-1L);
    else
	return PyInt_FromLong(x);

 onError:
    return NULL;
}

Py_C_Function( mxTextTools_setstrip,
	       "setstrip(text,set,start=0,stop=len(text),mode=0)\n\n"
	       "Strip all characters in text[start:stop] appearing in set.\n"
	       "mode indicates where to strip (<0: left; =0: left and right;\n"
	       ">0: right). set must be a string obtained with set()\n"
	       "DEPRECATED: use CharSet().strip() instead."
	       )
{
    char *tx;
    Py_ssize_t tx_len;
    char *setstr;
    Py_ssize_t setstr_len;
    Py_ssize_t start = 0;
    Py_ssize_t stop = INT_MAX;
    int mode = 0;
    
    Py_Get7Args("s#s#|iii:setstip",
		tx,tx_len,setstr,setstr_len,start,stop,mode);

    return mxTextTools_SetStrip(tx, tx_len,
				setstr, setstr_len,
				start, stop, 
				mode);

 onError:
    return NULL;
}

Py_C_Function( mxTextTools_setsplit,
	       "setsplit(text,set,start=0,stop=len(text))\n\n"
	       "Split text[start:stop] into substrings using set,\n"
	       "omitting the splitting parts and empty substrings.\n"
	       "set must be a string obtained from set()\n"
	       "DEPRECATED: use CharSet().split() instead."
	       )
{
    char *tx;
    Py_ssize_t tx_len;
    char *setstr;
    Py_ssize_t setstr_len;
    Py_ssize_t start = 0;
    Py_ssize_t stop = INT_MAX;

    Py_Get6Args("s#s#|ii:setsplit",
		tx,tx_len,setstr,setstr_len,start,stop);

    return mxTextTools_SetSplit(tx, tx_len,
				setstr, setstr_len,
				start, stop);
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_setsplitx,
	       "setsplitx(text,set,start=0,stop=len(text))\n\n"
	       "Split text[start:stop] into substrings using set, so\n"
	       "that every second entry consists only of characters in set.\n"
	       "set must be a string obtained with set()\n"
	       "DEPRECATED: use CharSet().splitx() instead."
	       )
{
    Py_ssize_t text_len = INT_MAX;
    Py_ssize_t start = 0;
    char *tx;
    Py_ssize_t tx_len;
    char *setstr;
    Py_ssize_t setstr_len;

    Py_Get6Args("s#s#|ii:setsplitx",
		tx,tx_len,setstr,setstr_len,start,text_len);

    return mxTextTools_SetSplitX(tx, tx_len,
				 setstr, setstr_len,
				 start, text_len);
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_upper,
	       "upper(text)\n\n"
	       "Return text converted to upper case.")
{
    PyObject *text;
    
    Py_GetArgObject(text);
    if (PyString_Check(text))
	return mxTextTools_Upper(text);
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text))
	return mxTextTools_UnicodeUpper(text);
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");

 onError:
    return NULL;
}

Py_C_Function( mxTextTools_lower,
	       "lower(text)\n\n"
	       "Return text converted to lower case.")
{
    PyObject *text;
    
    Py_GetArgObject(text);
    if (PyString_Check(text))
	return mxTextTools_Lower(text);
#ifdef HAVE_UNICODE
    else if (PyUnicode_Check(text))
	return mxTextTools_UnicodeLower(text);
#endif
    else
	Py_Error(PyExc_TypeError,
		 "expected string or unicode");
    
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_str2hex,
	       "str2hex(text)\n\n"
	       "Return text converted to a string consisting of two byte\n"
	       "HEX values.")
{
    char *str;
    Py_ssize_t len;
    
    Py_Get2Args("s#",str,len);

    return mxTextTools_HexStringFromString(str,len);
    
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_hex2str,
	       "hex2str(text)\n\n"
	       "Return text interpreted as two byte HEX values converted\n"
	       "to a string.")
{
    char *str;
    Py_ssize_t len;
    
    Py_Get2Args("s#",str,len);

    return mxTextTools_StringFromHexString(str,len);
    
 onError:
    return NULL;
}

Py_C_Function( mxTextTools_isascii,
	       "isascii(text,start=0,stop=len(text))\n\n"
	       "Return 1/0 depending on whether text only contains ASCII\n"
	       "characters."
	       )
{
    PyObject *text;
    Py_ssize_t start=0, stop = INT_MAX;
    int rc;
    
    Py_GetArgObject(text);
    rc = mxTextTools_IsASCII(text, start, stop);
    if (rc < 0)
	goto onError;
    return PyInt_FromLong(rc);
    
 onError:
    return NULL;
}

/* --- module init --------------------------------------------------------- */

/* Python Method Table */

static PyMethodDef Module_methods[] =
{   
    Py_MethodWithKeywordsListEntry("tag",mxTextTools_tag),
    Py_MethodListEntry("join",mxTextTools_join),
    Py_MethodListEntry("cmp",mxTextTools_cmp),
    Py_MethodListEntry("joinlist",mxTextTools_joinlist),
    Py_MethodListEntry("set",mxTextTools_set),
    Py_MethodListEntry("setfind",mxTextTools_setfind),
    Py_MethodListEntry("setsplit",mxTextTools_setsplit),
    Py_MethodListEntry("setsplitx",mxTextTools_setsplitx),
    Py_MethodListEntry("setstrip",mxTextTools_setstrip),
    Py_MethodWithKeywordsListEntry("TextSearch",mxTextSearch_TextSearch),
    Py_MethodListEntry("CharSet",mxCharSet_CharSet),
    Py_MethodListEntry("TagTable",mxTagTable_TagTable),
#ifdef HAVE_UNICODE
    Py_MethodListEntry("UnicodeTagTable",mxTagTable_UnicodeTagTable),
#endif
    Py_MethodListEntrySingleArg("upper",mxTextTools_upper),
    Py_MethodListEntrySingleArg("lower",mxTextTools_lower),
    Py_MethodListEntry("charsplit",mxTextTools_charsplit),
    Py_MethodListEntry("splitat",mxTextTools_splitat),
    Py_MethodListEntry("suffix",mxTextTools_suffix),
    Py_MethodListEntry("prefix",mxTextTools_prefix),
    Py_MethodListEntry("hex2str",mxTextTools_hex2str),
    Py_MethodListEntry("str2hex",mxTextTools_str2hex),
    Py_MethodListEntrySingleArg("isascii",mxTextTools_isascii),
    {NULL,NULL} /* end of list */
};

/* Cleanup function */
static 
void mxTextToolsModule_Cleanup(void)
{
    mxTextTools_TagTables = NULL;

    /* Reset mxTextTools_Initialized flag */
    mxTextTools_Initialized = 0;
}

MX_EXPORT(void) 
     initmxTextTools(void)
{
    PyObject *module, *moddict;
    
    if (mxTextTools_Initialized)
	Py_Error(PyExc_SystemError,
		 "can't initialize "MXTEXTTOOLS_MODULE" more than once");

    /* Init type objects */
    PyType_Init(mxTextSearch_Type);
#ifdef MXFASTSEARCH
    PyType_Init(mxFS_Type);
#endif
    PyType_Init(mxCharSet_Type);
    PyType_Init(mxTagTable_Type);

    /* create module */
    module = Py_InitModule4(MXTEXTTOOLS_MODULE, /* Module name */
			    Module_methods, /* Method list */
			    Module_docstring, /* Module doc-string */
			    (PyObject *)NULL, /* always pass this as *self */
			    PYTHON_API_VERSION); /* API Version */
    if (!module)
	goto onError;

    /* Init TagTable cache */
    if ((mxTextTools_TagTables = PyDict_New()) == NULL)
	goto onError;

    /* Register cleanup function */
    if (Py_AtExit(mxTextToolsModule_Cleanup))
	/* XXX what to do if we can't register that function ??? */;

    /* Add some symbolic constants to the module */
    moddict = PyModule_GetDict(module);
    PyDict_SetItemString(moddict, 
			 "__version__",
			 PyString_FromString(VERSION));

    mx_ToUpper = mxTextTools_ToUpper();
    PyDict_SetItemString(moddict, 
			 "to_upper",
			 mx_ToUpper);

    mx_ToLower = mxTextTools_ToLower();
    PyDict_SetItemString(moddict, 
			 "to_lower",
			 mx_ToLower);

    /* Let the tag table cache live in the module dictionary; we just
       keep a weak reference in mxTextTools_TagTables around. */
    PyDict_SetItemString(moddict, 
			 "tagtable_cache",
			 mxTextTools_TagTables);
    Py_DECREF(mxTextTools_TagTables);

    insint(moddict, "BOYERMOORE", MXTEXTSEARCH_BOYERMOORE);
    insint(moddict, "FASTSEARCH", MXTEXTSEARCH_FASTSEARCH);
    insint(moddict, "TRIVIAL", MXTEXTSEARCH_TRIVIAL);
  
    /* Init exceptions */
    if ((mxTextTools_Error = insexc(moddict,
				    "Error",
				    PyExc_StandardError)) == NULL)
	goto onError;

    /* Type objects */
    Py_INCREF(&mxTextSearch_Type);
    PyDict_SetItemString(moddict, "TextSearchType",
			 (PyObject *)&mxTextSearch_Type);
    Py_INCREF(&mxCharSet_Type);
    PyDict_SetItemString(moddict, "CharSetType",
			 (PyObject *)&mxCharSet_Type);
    Py_INCREF(&mxTagTable_Type);
    PyDict_SetItemString(moddict, "TagTableType",
			 (PyObject *)&mxTagTable_Type);

    /* Tag Table command symbols (these will be exposed via
       simpleparse.stt.TextTools.Constants.TagTables) */
    insint(moddict, "_const_AllIn", MATCH_ALLIN);
    insint(moddict, "_const_AllNotIn", MATCH_ALLNOTIN);
    insint(moddict, "_const_Is", MATCH_IS);
    insint(moddict, "_const_IsIn", MATCH_ISIN);
    insint(moddict, "_const_IsNot", MATCH_ISNOTIN);
    insint(moddict, "_const_IsNotIn", MATCH_ISNOTIN);

    insint(moddict, "_const_Word", MATCH_WORD);
    insint(moddict, "_const_WordStart", MATCH_WORDSTART);
    insint(moddict, "_const_WordEnd", MATCH_WORDEND);

    insint(moddict, "_const_AllInSet", MATCH_ALLINSET);
    insint(moddict, "_const_IsInSet", MATCH_ISINSET);
    insint(moddict, "_const_AllInCharSet", MATCH_ALLINCHARSET);
    insint(moddict, "_const_IsInCharSet", MATCH_ISINCHARSET);

    insint(moddict, "_const_Fail", MATCH_FAIL);
    insint(moddict, "_const_Jump", MATCH_JUMP);
    insint(moddict, "_const_EOF", MATCH_EOF);
    insint(moddict, "_const_Skip", MATCH_SKIP);
    insint(moddict, "_const_Move", MATCH_MOVE);

    insint(moddict, "_const_JumpTarget", MATCH_JUMPTARGET);

    insint(moddict, "_const_sWordStart", MATCH_SWORDSTART);
    insint(moddict, "_const_sWordEnd", MATCH_SWORDEND);
    insint(moddict, "_const_sFindWord", MATCH_SFINDWORD);
    insint(moddict, "_const_NoWord", MATCH_NOWORD);

    insint(moddict, "_const_Call", MATCH_CALL);
    insint(moddict, "_const_CallArg", MATCH_CALLARG);

    insint(moddict, "_const_Table", MATCH_TABLE);
    insint(moddict, "_const_SubTable", MATCH_SUBTABLE);
    insint(moddict, "_const_TableInList", MATCH_TABLEINLIST);
    insint(moddict, "_const_SubTableInList", MATCH_SUBTABLEINLIST);

    insint(moddict, "_const_Loop", MATCH_LOOP);
    insint(moddict, "_const_LoopControl", MATCH_LOOPCONTROL);

    /* Tag Table command flags */
    insint(moddict, "_const_CallTag", MATCH_CALLTAG);
    insint(moddict, "_const_AppendToTagobj", MATCH_APPENDTAG);
    insint(moddict, "_const_AppendTagobj", MATCH_APPENDTAGOBJ);
    insint(moddict, "_const_AppendMatch", MATCH_APPENDMATCH);
    insint(moddict, "_const_LookAhead", MATCH_LOOKAHEAD);

    /* Tag Table argument integers */
    insint(moddict, "_const_To", MATCH_JUMP_TO);
    insint(moddict, "_const_MatchOk", MATCH_JUMP_MATCHOK);
    insint(moddict, "_const_MatchFail", MATCH_JUMP_MATCHFAIL);
    insint(moddict, "_const_ToEOF", MATCH_MOVE_EOF);
    insint(moddict, "_const_ToBOF", MATCH_MOVE_BOF);
    insint(moddict, "_const_Here", MATCH_FAIL_HERE);

    insint(moddict, "_const_ThisTable", MATCH_THISTABLE);

    insint(moddict, "_const_Break", MATCH_LOOPCONTROL_BREAK);
    insint(moddict, "_const_Reset", MATCH_LOOPCONTROL_RESET);

    DPRINTF("sizeof(string_charset)=%i bytes\n", sizeof(string_charset));
#ifdef HAVE_UNICODE
    DPRINTF("sizeof(unicode_charset)=%i bytes\n", sizeof(unicode_charset));
#endif

    /* We are now initialized */
    mxTextTools_Initialized = 1;

 onError:
    /* Check for errors and report them */
    if (PyErr_Occurred())
	Py_ReportModuleInitError(MXTEXTTOOLS_MODULE);
    return;
}
