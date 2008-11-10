/* recursive tag-table commands */
	
case MATCH_TABLE:
case MATCH_SUBTABLE:
case MATCH_TABLEINLIST:
case MATCH_SUBTABLEINLIST:
	{
		PyObject * newTable = NULL;

		if (childReturnCode == NULL_CODE ) {
			/* haven't yet parsed the sub-table match */
			switch (command) {
				/* determine the table to which we will transfer control */
				case MATCH_TABLE:
				case MATCH_SUBTABLE:
					{
						/* switch to either current tag table or a compiled sub-table */
						if (PyInt_Check(match) && 
							PyInt_AS_LONG(match) == MATCH_THISTABLE) {
								newTable = (PyObject *)table;
						} else {
							newTable = match;
						}

						/* XXX Fix to auto-compile that match argument 

						Should also test that it _is_ a compiled TagTable,
						rather than that it _isn't_ a tuple?
						*/
						if (!mxTagTable_Check(newTable)) { 
							childReturnCode = ERROR_CODE;
							errorType = PyExc_TypeError;
							errorMessage = PyString_FromFormat(
								 "Match argument must be compiled TagTable: was a %.50s",
								 newTable->ob_type->tp_name
							);
						} else {
							/* we decref in POP */
							Py_INCREF(newTable);
						}
						break;
					}
				case MATCH_TABLEINLIST:
				case MATCH_SUBTABLEINLIST:
					{
						/* switch to explicitly specified table in a list (compiling if necessary) */


						newTable = PyList_GetItem(
							PyTuple_GET_ITEM(match, 0),
							PyInt_AS_LONG(
								PyTuple_GET_ITEM(match, 1)
							)
						);
						if (newTable == NULL) {
							childReturnCode = ERROR_CODE;
							errorType = PyExc_TypeError;
							errorMessage = PyString_FromFormat(
								"Tag table entry %d: Could not find target table in list of tables",
								(unsigned int)index
							);
						} else {
							if (mxTagTable_Check(newTable)) {
								/* This is decref'd in POP */
								Py_INCREF(newTable);
							} else {
								/* These tables are considered to be
								   cacheable. */
								newTable = mxTagTable_New(newTable,
										   table->tabletype,
										   1);
								/* why didn't we increment the refcount here? does New give us a new ref? */
								if (newTable == NULL) {
									childReturnCode = ERROR_CODE;
									errorType = PyExc_TypeError;
									errorMessage = PyString_FromFormat(
										"Tag table entry %d: Could not compile target table",
										(unsigned int)index
									);
								}
							}
						}
						break;
					}

			}

			if (childReturnCode == NULL_CODE) { 
				/* we found a valid newTable */
				PyObject *subtags = NULL;

				if (taglist != Py_None && command != MATCH_SUBTABLE && command != MATCH_SUBTABLEINLIST) {
					/* Create a new list for use as subtaglist 
					
						Will be decref'd by the child-finished clause if necessary
					*/
					subtags = PyList_New(0);
					if (subtags == NULL) {
						childReturnCode = ERROR_CODE;
						errorType = PyExc_MemoryError;
					}
				} else {
					/* Use taglist as subtaglist 

					We don't incref it as we check explicitly for whether
					it's the same when we go to decref (during childReturnCode 
					handling)
					*/
					subtags = taglist;
				}

				/* match other table */
				PUSH_STACK( newTable, subtags );
				RESET_TABLE_VARIABLES
			}
		} 
		break;
	}
	    
