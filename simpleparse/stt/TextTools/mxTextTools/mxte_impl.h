/*
  mxte_impl -- A table driven Tagging Engine for Python (Version 0.9)

  This is the Tagging Engine implementation. It can be compiled for
  8-bit strings and Unicode by setting the TE_* defines appropriately.

  Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com
  Copyright (c) 2003-2006, Mike Fletcher; mailto:mcfletch@vrplumber.com
*/

#ifndef TE_STRING_CHECK 
# define TE_STRING_CHECK(obj) PyString_Check(obj)
#endif
#ifndef TE_STRING_AS_STRING
# define TE_STRING_AS_STRING(obj) PyString_AS_STRING(obj)
#endif
#ifndef TE_STRING_GET_SIZE
# define TE_STRING_GET_SIZE(obj) PyString_GET_SIZE(obj)
#endif
#ifndef TE_STRING_FROM_STRING
# define TE_STRING_FROM_STRING(str, size) PyString_FromStringAndSize(str, size)
#endif
#ifndef TE_CHAR
# define TE_CHAR char
#endif
#ifndef TE_HANDLE_MATCH
# define TE_HANDLE_MATCH string_match_append
#endif
#ifndef TE_ENGINE_API
# define TE_ENGINE_API mxTextTools_TaggingEngine
#endif


/* --- Tagging Engine ----------------------------------------------------- */
/*  Non-recursive restructuring by Mike Fletcher to support SimpleParse

  This restructuring eliminates the use of the C call stack for
  processing sub-table and table directives, allowing these to be
  used for repetition calls if desired.


while 1:
	while (index_in_table() and returnCode == NULL_CODE):
		decode the current table[index]
		if the current tag is new (not already processed):
			reset tag variables
			switch( tag command ):
				do what tag wants to do()
				set tag-related variables
				set childReturnCode (tag variable)
				if table:
					push_frame_stack()
					set childReturnCode == PENDING
		switch(childReturnCode):
			# figure out what to do with child's results
			# possibly set table-wide returnValue
			childSuccess
				append values
				update table-wide values
				set new index
			childFailure
				rewind position
				set new index
			childError
				signal error for whole table
			childPending
				ignore/continue processing without updating list values
		reset childReturnCode
	#done table, figure out what to do now...
	if no explicit return value:
		figure out implicit
	if failure:
		truncate result list to previous length
		reset position
	if error:
		report error as exception
		exit
	else:
		if frame_stack():
			pop_frame_stack()
		else:
			return result

*/

/* call-stack structures used in non-recursive implementation */
#ifndef TEXTTOOLS_CALL_STACK_STRUCTURES
# define TEXTTOOLS_CALL_STACK_STRUCTURES

/* codes for returnCode and childReturnCode variables */
#define EOF_CODE 3
#define SUCCESS_CODE 2
#define FAILURE_CODE 1
#define ERROR_CODE 0
#define NULL_CODE -1
#define PENDING_CODE -2

typedef struct stack_entry {
	/* represents data stored for a particular stack recursion
	
	  We want to make this as small as possible, so anything that
	  is duplicate information (such as unpacked values of the tag or table)
	  is ignored.

	  Eventually this may support another field "available branches"
	  recording backtracking points for the engine.
	*/
	void * parent; /* pointer to a parent table or NULL */


	Py_ssize_t position; /* where the engine is currently parsing for the parent table*/
	Py_ssize_t startPosition; /* position where we started parsing for the parent table */

	mxTagTableObject * table; /* the parent table */
	Py_ssize_t index; /* index of the child tag in the parent table */

	Py_ssize_t childStart; /* text start position for the child table */
	PyObject * results; /* the result-target of the parent */
	Py_ssize_t resultsLength; /* the length of the results list before the sub-table is called */
} recursive_stack_entry;


/* Macro to reset table-specific variables 

XXX Not sure if loop vars are table or tag specific
*/
#define RESET_TABLE_VARIABLES {\
	index=0;\
	table_len = table->numentries;\
	returnCode = NULL_CODE;\
	loopcount = -1;\
	loopstart = startPosition;\
	taglist_len = PyList_Size( taglist );\
}

/* Macro to reset tag-specific variables 

*/
#define RESET_TAG_VARIABLES {\
	childStart = position;\
	childPosition = position;\
	childReturnCode = NULL_CODE;\
	childResults = NULL;\
}
/* Macro to decode a tag-entry into local variables */
#define DECODE_TAG {\
	mxTagTableEntry *entry;\
	entry = &table->entry[index];\
	command = entry->cmd;\
	flags = entry->flags;\
	match = entry->args;\
	failureJump = entry->jne;\
	successJump = entry->je;\
	tagobj = entry->tagobj;\
	if (tagobj == NULL) { tagobj = Py_None;}\
}

/* macro to push relevant local variables onto the stack and setup for child table
	newTable becomes table, newResults becomes taglist

	This is currently only called in the Table/SubTable family of commands,
	could be inlined there, but I find it cleaner to read here.
*/
#define PUSH_STACK( newTable, newResults ) {\
	stackTemp = (recursive_stack_entry *) PyMem_Malloc( sizeof( recursive_stack_entry ));\
	stackTemp->parent = stackParent;\
	stackTemp->position = position;\
	stackTemp->startPosition = startPosition;\
	stackTemp->table = table;\
	stackTemp->index = index;\
	stackTemp->childStart = childStart;\
	stackTemp->resultsLength = taglist_len;\
	stackTemp->results = taglist;\
	\
	stackParent = stackTemp;\
	childReturnCode = PENDING_CODE;\
	\
	startPosition = position;\
	table = (mxTagTableObject *) newTable;\
	taglist = newResults;\
}
#define POP_STACK {\
	if (stackParent) {\
		childStart = stackParent->childStart;\
		childPosition = position;\
		position = stackParent->position;\
		\
		startPosition = stackParent->startPosition;\
		\
		childResults = taglist;\
		taglist_len = stackParent->resultsLength;\
		taglist = stackParent->results;\
		if (table != stackParent->table ) { Py_DECREF( table ); }\
		table = stackParent->table;\
		table_len = table->numentries;\
		index = stackParent->index;\
		\
		stackTemp = stackParent->parent;\
		PyMem_Free( stackParent );\
		stackParent = stackTemp;\
		stackTemp = NULL;\
		\
		childReturnCode = returnCode;\
		returnCode = NULL_CODE;\
	}\
}


#endif

/* mxTextTools_TaggingEngine(): a table driven parser engine
   
   - return codes: returnCode = 2: match ok; returnCode = 1: match failed; returnCode = 0: error
   - doesn't check type of passed arguments !
   - doesn't increment reference counts of passed objects !
*/



int TE_ENGINE_API(
	PyObject *textobj,
	Py_ssize_t sliceleft,	
	Py_ssize_t sliceright,	
	mxTagTableObject *table,
	PyObject *taglist,
	PyObject *context,
	Py_ssize_t *next
) {
    TE_CHAR *text = NULL;		/* Pointer to the text object's data */

	/* local variables pushed into stack on recurse */
		/* whole-table variables */
		Py_ssize_t position = sliceleft;		/* current (head) position in text for whole table */
		Py_ssize_t startPosition = sliceleft;	/* start position for current tag */
		Py_ssize_t table_len = table->numentries; /* table length */
		short returnCode = NULL_CODE;		/* return code: -1 not set, 0 error, 1
					   not ok, 2 ok */
		Py_ssize_t index=0; 			/* index of current table entry */
		Py_ssize_t taglist_len = PyList_Size( taglist );


		/* variables tracking status of the current tag */
		register short childReturnCode = NULL_CODE; /* the current child's return code value */
		Py_ssize_t childStart = startPosition;
		register Py_ssize_t childPosition = startPosition;
		PyObject *childResults = NULL; /* store's the current child's results (for table children) */
		int flags=0;			/* flags set in command */
		int command=0;			/* command */
		int failureJump=0;			/* rel. jump distance on 'not matched', what should the default be? */
		int successJump=1;			/* dito on 'matched', what should the default be? */
		PyObject *match=NULL;		/* matching parameter */
		int loopcount = -1; 	/* loop counter */
		Py_ssize_t loopstart = startPosition;	/* loop start position */
		PyObject *tagobj = NULL;


	/* parentTable is our nearest parent, i.e. the next item to pop
	   off the processing stack.  We copied our local variables to it
	   before starting a child table, and will copy back from it when
	   we finish the child table.  It's normally NULL
	*/
	recursive_stack_entry * stackParent = NULL;
	recursive_stack_entry * stackTemp = NULL; /* just temporary storage for parent pointers */

	/* Error-management variables */
	PyObject * errorType = NULL;
	PyObject * errorMessage = NULL;

    /* Initialise the buffer
	
	Here is where we will add memory-mapped file support I think...

		expand the TE_STRING macros to check for mmap file objects
		(only for str-type) and to access their values appropriately
			f = open('c:\\temp\\test.mem', 'r')
			buffer = mmap.mmap( f.fileno(), 0, access = mmap.ACCESS_READ )

	*/
	if (!TE_STRING_CHECK(textobj)) {
		returnCode = ERROR_CODE;
		errorType = PyExc_TypeError;
		errorMessage = PyString_FromFormat(
		     "Expected a string or unicode object to parse: found %.50s",
		     Py_TYPE(textobj)->tp_name
		);
	} else {
	    text = TE_STRING_AS_STRING(textobj);
		if (text == NULL) {
			returnCode = ERROR_CODE;
		}
	}

	while (1) {
		/* this loop processes a whole table */
		while (
			(index < table_len) &
			(returnCode == NULL_CODE) &
			(index >= 0)
		) {
			DPRINTF( "index %i\n", index );
			DECODE_TAG
			if (childReturnCode == NULL_CODE ) {
				/* if we are not continuing processing of the child
				   from a previous iteration we need to unpack the
				   child into local variables
				*/
				RESET_TAG_VARIABLES
				childStart = position;
				childPosition = position;

			}
			if (command < MATCH_MAX_LOWLEVEL) {
#include "lowlevelcommands.h"
			} else {
				switch (command) {
/* Jumps & special commands */
#include "speccommands.h"
/* non-table-recursion high-level stuff */
#include "highcommands.h"
/* the recursive table commands */
#include "recursecommands.h"
					default:
						{
							childReturnCode = ERROR_CODE;
							errorType = PyExc_ValueError;
							errorMessage = PyString_FromFormat(
								 "Unrecognised command code %i",
								 command
							);
						}
				}
			}
			/* we're done a single tag, process partial results for the current child 

				This is a major re-structuring point.  Previously
				all of this was scattered around (and duplicated among)
				the various command and command-group clauses.

				There also used to be a function call to handle the
				append/call functions.  That's now handled inline

			*/
			/* sanity check wanted by Marc-André for skip-before-buffer */
			if (childPosition < 0) {
				childReturnCode = ERROR_CODE;
				errorType = PyExc_TypeError;
				errorMessage = PyString_FromFormat(
					 "tagobj (type %.50s) table entry %d moved/skipped beyond start of text (to position %d)",
					 Py_TYPE(tagobj)->tp_name,
					 (unsigned int)index,
					 (unsigned int)childPosition
				);
			}
			DPRINTF( "switch on return code %i\n", childReturnCode );
			switch(childReturnCode) {
				case NULL_CODE:
				case SUCCESS_CODE:
					/* childReturnCode wasn't set or we positively matched 

					positions are always:
						childStart, childPosition
					sub-results are:
						childResults
							unless childResults is taglist
								in which case we use Py_None  for the tag's children
							unless childResults is NULL
								in which case we create an empty list object

					we call:
						tagobj == Py_None :
							do nothing...

						[ result tuple needed ]
							CallTag:
								entry->tagobj( resultTuple )
							AppendToTagobj:
								entry->tagobj.append( resultTuple )
							General Case:
								taglist.append( resultTuple )

						AppendMatch:
							taglist.append( text[childStart:childPosition] )
						AppendTagobj:
							taglist.append( entry->tagobj )

					if LookAhead is specified:
						childPosition is set to childStart before continuing

					finally we set position = childPosition
					*/
					{
						PyObject * objectToCall = NULL;
						PyObject * objectCallResult = NULL;
						int releaseCallObject = 0;
						int releaseChildResults = 0;
						int releaseParameter = 1;
						PyObject * parameter = NULL;
						DPRINTF( "finishing success-code or null \n" );

						if (tagobj == Py_None  ) {
							/* XXX note: this short-circuits around "AppendTagobj" flagged items which
							specified tagobj == None... don't know if that's wanted or not.  Similarly
							doesn't report AppendMatch's.  Not sure what's appropriate there either.
							*/
							DPRINTF( "tagobj was none\n" );
							DPRINTF( "Matched %i:%i but result not saved", childStart, childPosition );
						} else {
							/* get the callable object */
							/* normally it's taglist.append, do the exceptions first */
							DPRINTF( "tagobj non-None, finding callable\n" );
							if (flags & MATCH_CALLTAG) {
								/* want the tag itself */
								objectToCall = tagobj;
							} else if (flags & MATCH_APPENDTAG) {
								/* AppendToTagobj -> want the tag's append method */
								DPRINTF( "append to tag obj\n" );
								objectToCall = PyObject_GetAttrString( tagobj, "append" );
								DPRINTF( "got object\n");
								if (objectToCall == NULL) {
									DPRINTF( "got invalid object\n");
									returnCode = ERROR_CODE;
									errorType = PyExc_AttributeError;
									errorMessage = PyString_FromFormat(
										 "tagobj (type %.50s) for table entry %d (flags include AppendTag) doesn't have an append method",
										 Py_TYPE(tagobj)->tp_name,
										 (unsigned int)index
									);
								} else {
									DPRINTF( "got valid object\n");
									releaseCallObject = 1;
								}
							} else {
								DPRINTF( "appending to tag-list\n" );
								/* append of the taglist, which we know exists, because it's a list
								We optimise this to use the raw List API
								*/
								objectToCall = NULL; /*PyObject_GetAttrString( taglist, "append" );*/
							}
							if (returnCode == NULL_CODE && objectToCall && PyCallable_Check(objectToCall)==0) {
								/* object to call isn't callable */
								DPRINTF( "object not callable\n" );
								returnCode = ERROR_CODE;
								errorType = PyExc_TypeError;
								errorMessage = PyString_FromFormat(
									 "The object to call type(%.50s) for table entry %d isn't callable",
									 Py_TYPE(objectToCall)->tp_name,
									 (unsigned int)index
								);
							}
							if (returnCode == NULL_CODE) {
								/* get the parameter with which to call */
								/* normally it's a result tuple, do exceptions first */
								DPRINTF( "getting parameter\n" );
								if (flags & MATCH_APPENDMATCH) {
									/* XXX need to do bounds-checking here
									so that:
										childStart >= sliceleft
										childPosition >= sliceleft
										childPosition <= sliceright
									*/
									/* MATCH_APPENDMATCH cannot occur with any
									other flag (makes no sense) so objectToCall
									_must_ be the taglist, and we just want to append
									the string, not a tuple wrapping the string.  That is,
									everywhere else we use tuples, here we don't
									*/
									parameter = TE_STRING_FROM_STRING(
										TE_STRING_AS_STRING(textobj) + childStart,
										childPosition - childStart
									);
									if (parameter == NULL) {
										/* error occured getting parameter, report the exception */
										returnCode = ERROR_CODE;
									}
								} else if ( flags & MATCH_APPENDTAGOBJ) {
									/* append the tagobj itself to the results list */
									if (tagobj == NULL) {
										parameter = Py_None;
									} else {
										parameter = tagobj;
									}
									releaseParameter = 0;
								} else {
									/* need to know what the child-list is to build resultsTuple
									if childResults is non-null and not taglist use it
									if childResults == taglist, use Py_None
									otherwise use Py_None ( originally we created a new empty list object, that was wrong :) ).
									*/
									if (childResults == taglist) {
										childResults = Py_None ;
									} else if (childResults != NULL) {
										/* exists already, with a reference from PUSH's creation */
										releaseChildResults = 1;
									} else {
										/* turns out mxTextTools declares the return value to be
										None or [], using None is far more efficient, so I've made
										the code use it here */
										childResults = Py_None;
										releaseChildResults = 0; /* we aren't increfing it locally */
									}
									if (childResults == NULL || tagobj == NULL) {
										returnCode = ERROR_CODE;
									} else {
										if (flags & MATCH_CALLTAG) {
											parameter = Py_BuildValue( "OOiiO", taglist, textobj, childStart, childPosition, childResults );
										} else if (flags & MATCH_APPENDTAG) {
											/* AppendToTagobj -> want to call append with a 4-tuple of values, so parameter needs to be ((x,y,z,w),) */
											/* XXX can't get the darn thing to accept "((OiiO))" :( */
											parameter = Py_BuildValue(
												"((OiiO))",
												Py_None,
												childStart,
												childPosition,
												childResults
											);
										} else {
											/* either we are calling a method that requires the 4 args, or we're appending the 4-tuple to a list */
											parameter = Py_BuildValue( "OiiO", tagobj, childStart, childPosition, childResults );
										}
										if (parameter == NULL) {
											returnCode = ERROR_CODE;
										}
									}
								}
								DPRINTF( "done getting parameter\n" );
								if (parameter == NULL && returnCode == ERROR_CODE && errorType == NULL) {
									errorType = PyExc_SystemError;
									/* following may fail, as we may have run out of memory */
									errorMessage = PyString_FromFormat(
										 "Unable to build return-value tuple"
									);
								}
								/* now have both object and parameter and object is callable */
								if (returnCode == NULL_CODE) {
									/* no errors yet */
									DPRINTF( "doing call\n" );
									if (objectToCall) {
										DPRINTF( " object call\n" );
										/* explicit object to call */
										Py_INCREF( objectToCall );
										Py_INCREF( parameter );
										DPRINTF( " lock released\n" );
										objectCallResult =  PyEval_CallObject( objectToCall, parameter );
										DPRINTF( " call finished\n" );
										Py_DECREF( objectToCall );
										Py_DECREF( parameter );
										DPRINTF( " lock acquired\n" );
										if (objectCallResult == NULL) {
											DPRINTF( " null result\n" );
											returnCode = ERROR_CODE;
											/* exception is already there, should alter error-handler to check for it */
										} else {
											DPRINTF( " non-null result, decrefing\n" );
											Py_DECREF( objectCallResult );
											DPRINTF( " decrefd\n" );
										}
										objectCallResult = NULL;
									} else {
										/* list steals reference */
										DPRINTF( " list append\n" );
										if (PyList_Append( taglist, parameter ) == -1) {
											returnCode = ERROR_CODE;
											/* list didn't steal ref yet */
											errorType = PyExc_SystemError;
											/* following is likely to fail, as we've likely run out of memory */
											errorMessage = PyString_FromFormat(
												 "Unable to append result tuple to result list!"
											);
										}
									}
								}
							}
							DPRINTF( "checking whether to release object\n" );
							if (releaseCallObject) {
								Py_DECREF( objectToCall );
							}
							objectToCall = NULL;
							releaseCallObject = 0;

							if (releaseChildResults) {
								Py_DECREF( childResults );
							}
							childResults = NULL;
							releaseChildResults = 0;
							if (releaseParameter && parameter ) {
								Py_DECREF( parameter );
							}
							parameter = NULL;
							releaseParameter = 1;
						} /* ends the else clause for reporting a result */
						/* reset for lookahead */
						if (flags & MATCH_LOOKAHEAD) {
							position = childStart;
						} else {
							position = childPosition;
						}
						index += successJump;
						DPRINTF( "finished success-handler code\n" );
						break;
					}
				case FAILURE_CODE:
					/* failed, if failure jump is default, should set table returnCode */
					if (childResults) {
						if (childResults != taglist) {
							/* different list, decref it since we won't be using it any more */
							Py_DECREF( childResults );
						}
						childResults = NULL;
					}
					/* XXX possible (eventual) logic error here?

						fail with jump of 0 might work in certain cases where the
						"parsing" is actually occuring outside of the current buffer
						(i.e. a side-effect-based parsing node that fails X times before
						finally succeeding).

						Don't see anything in current commands that can cause a problem
						but we may need to make this an explicitly watched idea, rather
						than a consequence of the child failing with a 0 failureJump value.
					*/
					position = childStart;
					if (failureJump == 0) {
						returnCode = 1;
					} else {
						index += failureJump;
					}
					break;
				case PENDING_CODE:
					/* the child tag hasn't begun parsing, this was a 
					recursive-tag-start loop pass. PENDING_CODE is set 
					by the stack push operation
					*/
					break;
				case ERROR_CODE: 
					{
						/* explicit error encountered while processing this child

							Handle this as gracefully as possible, potentially triggering
							huge sets of operations, but therefore needing to be very careful
							about system-level errors (such as memory errors).
							
							1) Signal whole table as err-d
							2) Record any extra values for the error message?
						*/
						returnCode = ERROR_CODE;
						break;
					}
				default:
					{
						/* what error should be raised when an un-recognised return code is generated? */
						returnCode = ERROR_CODE;
						errorType = PyExc_SystemError;
						errorMessage = PyString_FromFormat(
							 "An unknown child return code %i was generated by tag-table item %d",
							childReturnCode,
							(unsigned int)index
						);
					}
			}
			childReturnCode = NULL_CODE;
			/* single entry processing loop complete */
		}
		/* we're done the table, figure out what to do. */
		if (returnCode == NULL_CODE) {
			/* no explicit return code was set, but done table:
			
			index went beyond table_len (>=table_len) -> success
			index moved before table start (<= 0) -> failure
			*/
			if (index >= table_len) {
				/* success */
				returnCode = SUCCESS_CODE;
			} else if (position >= sliceright) {
				/* EOF while parsing, special type of failure 
				
				Eventually allow for returning the whole parse-stack
				for restarting the parser from a particular point.
				*/
				/*returnCode = EOF_CODE;*/
				returnCode = FAILURE_CODE;
			} else if (index < 0) {
				/* explicit jump before table */
				returnCode = FAILURE_CODE;
			} else {
				returnCode = FAILURE_CODE;
			}
		}
		if (returnCode == FAILURE_CODE) {
			/* truncate result list */
			if (PyList_SetSlice(
					taglist,
					taglist_len,
					PyList_Size(taglist),
					NULL)
			) {
				returnCode = ERROR_CODE;
				errorMessage = PyString_FromFormat(
					 "Unable to truncate list object (likely tagging engine error) type(%.50s)",
					 Py_TYPE(taglist)->tp_name
				);
			}
			/* reset position */
			position = startPosition;
		}
		if (returnCode == ERROR_CODE) {
			/* 
			DO_FANCY_ERROR_REPORTING( );

			This is where we will do the user-triggered error reporting
			(as well as reporting low-level errors such as memory/type/value).

			We have 3 values possibly available:
				errorType -> PyObject * to current error class (or NULL)
					if it is a MemoryError:

						Jettison some ballast then attempt to return a short
						message.  Need to create this ballast somewhere for that
						to work.

					if is any other error class:

						create the error object and raise it
						
						decorate it with details:

							current table (need to incref to keep alive)
							current index 
							current position
							childStart
							childPosition

						if it is simpleparse.stt.TextTools.ParsingError:
							(triggered by the user in their grammar)

							create a list of non-None parent tagobjs (a stack 
							report) and add it to the object

							



				3) Build an actual error object if possible?
				4) Report the parent hierarchy of the failure point
				5) 
			*/
			char * msg = NULL;
			if (errorMessage && errorType) {
				/* we only report our own error if we've got all the information for it 
				
				XXX Need to check that we don't have cases that are just setting type
				*/
				msg = PyString_AsString( errorMessage);
				PyErr_SetString( errorType, msg );
				Py_DECREF( errorMessage );
			}


			
			/* need to free the whole stack at once */
			while (stackParent != NULL) {
				/* this is inefficient, should do it all-in-one-go without copying values back 
				save for startPosition and returnCode in the last item*/
				POP_STACK
				/* need to clean up all INCREF'd objects as we go... */
				if (childResults != taglist) {
					/* different list, decref it since we won't be using it any more */
					Py_DECREF( childResults );
				}
				childResults = NULL;
			}
			*next = startPosition;
			return 0;
		} else {
			if (stackParent != NULL) {
				/* pop stack also sets the childReturnCode for us... */
				POP_STACK
			} else {
				/* this was the root table,
				 return the final results */
				if (returnCode == FAILURE_CODE) {
					/* there is a clause in the docs for tag that says
					this will return the "error position" for the table.
					That requires reporting childPosition for the the 
					last-matched position */
					*next = childPosition;
				} else {
					*next = position;
				}
				return returnCode;
			}
		}
	} /* end of infinite loop */
}

