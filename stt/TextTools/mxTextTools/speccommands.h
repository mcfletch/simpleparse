/*  "Special" commands code fragment

  The contract here is:

	The commands may alter any of the tag-specific variables

	errors may be indicated if encountered in childReturnCode and the error* variables

	setting childReturnCode equal to FAILURE_CODE declares that the read head has not moved

	childReturnCode must be set (or default "you have to move forward to match" semantics are used)

*/
/* doesn't there need to be a check for integer arguments
 that the value is an integer?
 Or does the compiler do that now */



	case MATCH_FAIL: /* == MATCH_JUMP */
		/* dumb question, what is MATCH_JUMP supposed to do? */
	    childReturnCode = FAILURE_CODE;
		break;
	    
	case MATCH_SKIP:
		/* Argh, what to do when moves past buffer?

		Where do we check that this is still in-bounds? 
		documented as always succeeding, but results in
		result-tuples with negative or out-of-range values 
		in current code.
		Can't do:
			if (childPosition < sliceleft) {
				childPosition = 0;
			} else if (childPosition > sliceright) {
				childPosition = sliceright;
			}
		because we might have another move, or an EOF
		or whatever coming up.

		Marc-André want's these conditions:
			childPosition < 0 { # (not sliceleft!)
				raise TypeError: Tag Table entry %(index): moved/skipped beyond start of text
			} and no check for > right or beyond end of buffer...
		*/
		DPRINTF("\nSkip %li characters\n"
			" in string    = '%.40s'\n",
			PyInt_AS_LONG(match),text+childPosition);
		childPosition += PyInt_AS_LONG(match);
	    childReturnCode = SUCCESS_CODE;
		break;

	case MATCH_MOVE:
		/* same potential out-of-bounds issue as with skip */
		childPosition = PyInt_AS_LONG(match);
		if (childPosition < 0) {
		    /* Relative to end of the slice */
		    childPosition += sliceright + 1;
		} else {
		    /* Relative to beginning of the slice */
		    childPosition += sliceleft;
		}
		DPRINTF("\nMove to position %i \n"
			" string       = '%.40s'\n",
			childPosition,text+childPosition);
	    childReturnCode = SUCCESS_CODE;
		break;
		
	case MATCH_EOF:
		DPRINTF("\nEOF at position %i ? \n"
			" string       = '%.40s'\n",
			childPosition,text+childPosition);

		if (sliceright > childPosition) { /* not matched */
			childReturnCode = FAILURE_CODE;
		} else {
			/* I don't see why this would necessarily be the end of the parsing run, after all
			you might want to match EOF, then back up X characters? The documentation doesn't
			mention anything about such a restriction.

			Approach here seems to match documentation functionality
			but still suffers the out-of-range problems seen in move
			and skip commands as well.
			*/
		    childReturnCode = SUCCESS_CODE;
		    childPosition = sliceright;
			childStart = sliceright;
		}
		break;


	case MATCH_JUMPTARGET:
		/* note: currently this can report a value, though I don't think
		that was intended originally.  I see it as useful because it lets
		you enter a flag in the results table just by specifying a non-None
		tagobj */
		/* null operation */
		DPRINTF("\nJumpTarget '%.40s' (skipped)\n",
			PyString_AsString(match));
	    childReturnCode = SUCCESS_CODE;
		break;
