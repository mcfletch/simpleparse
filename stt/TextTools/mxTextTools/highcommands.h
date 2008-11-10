/* non-recursive high-level commands 

  The contract here is:

	The commands may alter any of the tag-specific variables

	errors may be indicated if encountered in childReturnCode and the error* variables

*/

	case MATCH_SWORDSTART:
	case MATCH_SWORDEND:
	case MATCH_SFINDWORD:
		/* these items basically follow the low-level contract, with the
			only exception being that MATCH_SFINDWORD will change childStart
		*/
	    {
			Py_ssize_t wordstart, wordend;
			int returnCode;

			DPRINTF("\nsWordStart/End/sFindWord :\n"
				" in string   = '%.40s'\n",text+childPosition);
			childStart = childPosition;
			returnCode = TE_SEARCHAPI(
				match,
				text,
				childStart,
				sliceright,
				&wordstart,
				&wordend
			);
			if (returnCode < 0) {
				childReturnCode = ERROR_CODE;
				errorType = PyExc_SystemError;
				errorMessage = PyString_FromFormat(
					 "Search-object search returned value < 0 (%i): probable bug in text processing engine",
					 returnCode
				);
			} else if (returnCode == 0) { 
				/* not matched */
				DPRINTF(" (no success)\n");
				childReturnCode = FAILURE_CODE;
			} else { 
				/* matched, adjust childPosition according to the word start/end/find requirements */
				if (command == MATCH_SWORDSTART) {
					childPosition = wordstart;
				} else {
					childPosition = wordend;
				}
				if (command == MATCH_SFINDWORD) {
					/* XXX logic problem with lookahead
					should it reset to real childStart or 
					the fake one created here? */
					childStart = wordstart;
				}
				DPRINTF(" [%i:%i] (matched and remembered this slice)\n",
					childStart,childPosition);
			}
			break;
	    }

	case MATCH_LOOP:
		/* No clue what this is supposed to do, real surprising if it works...

		*/
	    DPRINTF("\nLoop: pre loop counter = %i\n",loopcount);
	    
		if (loopcount > 0) {
			/* we are inside a loop */
			loopcount--;
		} else if (loopcount < 0) {
			/* starting a new loop */
			if (PyInt_Check(match)) {
				loopcount = PyInt_AS_LONG(match);
				loopstart = childPosition;
			} else {
				childReturnCode = ERROR_CODE;
				errorType = PyExc_TypeError;
				errorMessage = PyString_FromFormat(
					 "Tag Table entry %d: expected an integer (command=Loop) got a %.50s",
					 (unsigned int)index,
					 match->ob_type->tp_name
				);
			}
		}
		if (childReturnCode == NULL_CODE ) {

			if (loopcount == 0) {
				/* finished loop */
				loopcount = -1;
			}
			if (loopstart == childPosition) {
				/* not matched */
				childReturnCode = FAILURE_CODE;
			} else {
				childReturnCode = SUCCESS_CODE;
				/* on success, add match from start of the whole loop to end of current iteration? 
				
				Would be really good if I had a clue what this is supposed to do :) .
				*/
				childStart = loopstart;
			}
			DPRINTF("\nloop: post loop counter = %i\n",loopcount);
		}
		break;

	case MATCH_LOOPCONTROL:

	    DPRINTF("\nLoopControl: loop counter = %i, "
		    "setting it to = %li\n",
		    loopcount,PyInt_AS_LONG(match));

	    loopcount = PyInt_AS_LONG(match);
		break;

	case MATCH_CALL:
	case MATCH_CALLARG:
		/* call and callarg actually follow the low-level contract */

	    {
			PyObject *fct = NULL;
			int argc = -1;
			
			if (!PyTuple_Check(match)) {
				argc = 0;
				fct = match;
			} else {
				argc = PyTuple_GET_SIZE(match) - 1;
				if (argc < 0) {
					/* how is this even possible? */
					childReturnCode = ERROR_CODE;
					errorType = PyExc_TypeError;
					errorMessage = PyString_FromFormat(
						"Tag Table entry %d: "
						"expected a tuple (fct,arg0,arg1,...)"
						"(command=CallArg)",
						(unsigned int)index
					);
				} else {
					fct = PyTuple_GET_ITEM(match,0);
				}
			}
			
			if (childReturnCode == NULL_CODE && PyCallable_Check(fct)) {
				PyObject *args;
				register PyObject *w;
				register Py_ssize_t argIndex;

				DPRINTF("\nCall[Arg] :\n");
			
				childStart = childPosition;

				/* Build args = (textobj,childStart,sliceright[,arg0,arg1,...]) */
				args = PyTuple_New(3 + argc);
				if (!args) {
					childReturnCode = ERROR_CODE;
					errorType = PyExc_SystemError;
					errorMessage = PyString_FromFormat(
						 "Unable to create argument tuple for CallArgs command at index %d",
						 (unsigned int)index
					);
				} else {
					Py_INCREF(textobj);
					PyTuple_SET_ITEM(args,0,textobj);
					w = PyInt_FromLong(childStart);
					if (!w){
						childReturnCode = ERROR_CODE;
						errorType = PyExc_SystemError;
						errorMessage = PyString_FromFormat(
							 "Unable to convert an integer %d to a Python Integer",
							 (unsigned int)childStart
						);
					} else {
						PyTuple_SET_ITEM(args,1,w);
						w = PyInt_FromLong(sliceright);
						if (!w) {
							childReturnCode = ERROR_CODE;
							errorType = PyExc_SystemError;
							errorMessage = PyString_FromFormat(
								 "Unable to convert an integer %d to a Python Integer",
								 (unsigned int)sliceright
							);
						} else {
							PyTuple_SET_ITEM(args,2,w);
							for (argIndex = 0; argIndex < argc; argIndex++) {
							w = PyTuple_GET_ITEM(match,argIndex + 1);
							Py_INCREF(w);
							PyTuple_SET_ITEM(args,3 + argIndex,w);
							}
							/* now actually call the object */
							w = PyEval_CallObject(fct,args);
							Py_DECREF(args);
							if (w == NULL) {
								childReturnCode = ERROR_CODE;
								/* child's error should be allowed to propagate */
							} else if (!PyInt_Check(w)) {
								childReturnCode = ERROR_CODE;
								errorType = PyExc_TypeError;
								errorMessage = PyString_FromFormat(
									 "Tag Table entry %d: matching function has to return an integer, returned a %.50s",
									 (unsigned int)index,
									 w->ob_type->tp_name
								);
							} else {
								childPosition = PyInt_AS_LONG(w);
								Py_DECREF(w);

								if (childStart == childPosition) { 
									/* not matched */
									DPRINTF(" (no success)\n");
									childReturnCode = FAILURE_CODE;
								}
							}
						}
					}
				}
			} else {
				childReturnCode = ERROR_CODE;
				errorType = PyExc_TypeError;
				errorMessage = PyString_FromFormat(
					"Tag Table entry %d: "
					"expected a callable object, got a %.50s"
					"(command=Call[Arg])",
					(unsigned int)index,
					fct->ob_type->tp_name
				);
			}
			break;
		}
