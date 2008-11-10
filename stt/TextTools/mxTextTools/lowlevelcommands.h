/* Low-level matching commands code fragment

  The contract here is:

	all commands move forward through the buffer
	
	failure to move forward indicates failure of the tag
	
	moving forward indicates success of the tag
	
	errors may be indicated if encountered in childReturnCode and the error* variables
	
	only childPosition should be updated otherwise

*/
TE_CHAR *m = TE_STRING_AS_STRING(match);
if (m == NULL) {
	childReturnCode = ERROR_CODE;
	errorType = PyExc_TypeError;
	errorMessage = PyString_FromFormat(
		 "Low-level command (%i) argument in entry %d couldn't be converted to a string object, is a %.50s",
		 command,
		 (unsigned int)index,
		 textobj->ob_type->tp_name

	);
} else {

switch (command) {

	case MATCH_ALLIN:

		{
			register Py_ssize_t ml = TE_STRING_GET_SIZE(match);
			register TE_CHAR *tx = &text[childPosition];

			DPRINTF("\nAllIn :\n"
				" looking for   = '%.40s'\n"
				" in string     = '%.40s'\n",m,tx);

			if (ml > 1) {
				for (; childPosition < sliceright; tx++, childPosition++) {
					register Py_ssize_t j;
					register TE_CHAR *mj = m;
					register TE_CHAR ctx = *tx;
					for (j=0; j < ml && ctx != *mj; mj++, j++) ;
					if (j == ml) break;
				}
			} else if (ml == 1) {
				/* one char only: use faster variant: */
				for (; childPosition < sliceright && *tx == *m; tx++, childPosition++) ;
			}
			break;
		}

	case MATCH_ALLNOTIN:

		{
			register Py_ssize_t ml = TE_STRING_GET_SIZE(match);
			register TE_CHAR *tx = &text[childPosition];

			DPRINTF("\nAllNotIn :\n"
				" looking for   = '%.40s'\n"
				" not in string = '%.40s'\n",m,tx);

			if (ml != 1) {
				for (; childPosition < sliceright; tx++, childPosition++) {
					register Py_ssize_t j;
					register TE_CHAR *mj = m;
					register TE_CHAR ctx = *tx;
					for (j=0; j < ml && ctx != *mj; mj++, j++) ;
					if (j != ml) break;
				}
			} else {
				/* one char only: use faster variant: */
				for (; childPosition < sliceright && *tx != *m; tx++, childPosition++) ;
			}
			break;
		}

	case MATCH_IS: 
		
		{
			DPRINTF("\nIs :\n"
				" looking for   = '%.40s'\n"
				" in string     = '%.40s'\n",m,text+childPosition);

			if (childPosition < sliceright && *(&text[childPosition]) == *m) {
				childPosition++;
			}
			break;
		}

	case MATCH_ISIN:

	{
		register Py_ssize_t ml = TE_STRING_GET_SIZE(match);
		register TE_CHAR ctx = text[childPosition];

		DPRINTF("\nIsIn :\n"
			" looking for   = '%.40s'\n"
			" in string     = '%.40s'\n",m,text+childPosition);

		if (ml > 0 && childPosition < sliceright) {
		register Py_ssize_t j;
		register TE_CHAR *mj = m;
		for (j=0; j < ml && ctx != *mj; mj++, j++) ;
		if (j != ml) childPosition++;
		}

		break;
	}

	case MATCH_ISNOTIN:

	{
		register Py_ssize_t ml = TE_STRING_GET_SIZE(match);
		register TE_CHAR ctx = text[childPosition];

		DPRINTF("\nIsNotIn :\n"
			" looking for   = '%.40s'\n"
			" not in string = '%.40s'\n",m,text+childPosition);

		if (ml > 0 && childPosition < sliceright) {
		register Py_ssize_t j;
		register TE_CHAR *mj = m;
		for (j=0; j < ml && ctx != *mj; mj++, j++) ;
		if (j == ml) childPosition++;
		}
		else
		childPosition++;

		break;
	}

	case MATCH_WORD:

	{
		Py_ssize_t ml1 = TE_STRING_GET_SIZE(match) - 1;
		register TE_CHAR *tx = &text[childPosition + ml1];
		register Py_ssize_t j = ml1;
		register TE_CHAR *mj = &m[j];

		DPRINTF("\nWord :\n"
			" looking for   = '%.40s'\n"
			" in string     = '%.40s'\n",m,&text[childPosition]);

		if (childPosition+ml1 >= sliceright) break;
		
		/* compare from right to left */
		for (; j >= 0 && *tx == *mj;
		 tx--, mj--, j--) ;

		if (j >= 0) /* not matched */
		childPosition = startPosition; /* reset */
		else
		childPosition += ml1 + 1;
		break;
	}

	case MATCH_WORDSTART:
	case MATCH_WORDEND:

	{
		Py_ssize_t ml1 = TE_STRING_GET_SIZE(match) - 1;

		if (ml1 >= 0) {
		register TE_CHAR *tx = &text[childPosition];
			
		DPRINTF("\nWordStart/End :\n"
			" looking for   = '%.40s'\n"
			" in string     = '%.40s'\n",m,tx);

		/* Brute-force method; from right to left */
		for (;;) {
			register Py_ssize_t j = ml1;
			register TE_CHAR *mj = &m[j];

			if (childPosition+j >= sliceright) {
			/* reached eof: no match, rewind */
			childPosition = startPosition;
			break;
			}

			/* scan from right to left */
			for (tx += j; j >= 0 && *tx == *mj; 
			 tx--, mj--, j--) ;
			/*
			DPRINTF("match text[%i+%i]: %c == %c\n",
					childPosition,j,*tx,*mj);
			*/

			if (j < 0) {
			/* found */
			if (command == MATCH_WORDEND) childPosition += ml1 + 1;
			break;
			}
			/* not found: rewind and advance one char */
			tx -= j - 1;
			childPosition++;
		}
		}

		break;
	}

#if (TE_TABLETYPE == MXTAGTABLE_STRINGTYPE)

	/* Note: These two only work for 8-bit set strings. */
	case MATCH_ALLINSET:

	{
		register TE_CHAR *tx = &text[childPosition];
		unsigned char *m = (unsigned char *)PyString_AS_STRING(match);

		DPRINTF("\nAllInSet :\n"
			" looking for   = set at 0x%lx\n"
			" in string     = '%.40s'\n",(long)match,tx);

		for (;
		 childPosition < sliceright &&
		 (m[((unsigned char)*tx) >> 3] & 
		  (1 << (*tx & 7))) > 0;
		 tx++, childPosition++) ;

		break;
	}

	case MATCH_ISINSET:

	{
		register TE_CHAR *tx = &text[childPosition];
		unsigned char *m = (unsigned char *)PyString_AS_STRING(match);

		DPRINTF("\nIsInSet :\n"
			" looking for   = set at 0x%lx\n"
			" in string     = '%.40s'\n",(long)match,tx);

		if (childPosition < sliceright &&
		(m[((unsigned char)*tx) >> 3] & 
		 (1 << (*tx & 7))) > 0)
		childPosition++;

		break;
	}

#endif

	case MATCH_ALLINCHARSET:

	{
		Py_ssize_t matching;

		DPRINTF("\nAllInCharSet :\n"
			" looking for   = CharSet at 0x%lx\n"
			" in string     = '%.40s'\n",
			(long)match, &text[childPosition]);
		
		matching = mxCharSet_Match(match,
					   textobj,
					   childPosition,
					   sliceright,
					   1);
		if (matching < 0) {
			childReturnCode = ERROR_CODE;
			errorType = PyExc_SystemError;
			errorMessage = PyString_FromFormat(
				 "Character set match returned value < 0 (%d): probable bug in text processing engine",
				 (unsigned int)matching
			);
		} else {
			childPosition += matching;
		}
		break;
	}

	case MATCH_ISINCHARSET:

		{
			int test;

			DPRINTF("\nIsInCharSet :\n"
				" looking for   = CharSet at 0x%lx\n"
				" in string     = '%.40s'\n",
				(long)match, &text[childPosition]);

#if (TE_TABLETYPE == MXTAGTABLE_STRINGTYPE)
			test = mxCharSet_ContainsChar(match, text[childPosition]);
#else
			test = mxCharSet_ContainsUnicodeChar(match, text[childPosition]);
#endif
			if (test < 0) {
				childReturnCode = ERROR_CODE;
				errorType = PyExc_SystemError;
				errorMessage = PyString_FromFormat(
					 "Character set match returned value < 0 (%i): probable bug in text processing engine",
					 test
				);
			} else if (test) {
				childPosition++;
			}
			break;
		}
	default:
		{
			childReturnCode = ERROR_CODE;
			errorType = PyExc_ValueError;
			errorMessage = PyString_FromFormat(
				 "Unrecognised Low-Level command code %i, maximum low-level code is %i",
				 command,
				 MATCH_MAX_LOWLEVEL
			);
		}
/* end of the switch, this child is finished */
}
} /* end of the wrapping if-check */

/* simple determination for these commands (hence calling them low-level) */
if (childReturnCode == NULL_CODE) {
	if (childPosition > childStart) {
		childReturnCode = SUCCESS_CODE;
	} else {
		childReturnCode = FAILURE_CODE;
	}
}
