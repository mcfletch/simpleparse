/* Low-level matching commands code fragment

  The contract here is:

	all commands move forward through the buffer

	failure to move forward indicates failure of the tag

	moving forward indicates success of the tag

	errors may be indicated if encountered in childReturnCode and the error* variables

	only childPosition should be updated otherwise

*/
/* Character access macros for Unicode vs bytes engines */
#ifdef GET_TEXT_CHAR
#undef GET_TEXT_CHAR
#endif
#ifdef GET_MATCH_CHAR
#undef GET_MATCH_CHAR
#endif

#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
#define GET_TEXT_CHAR(obj, pos) PyUnicode_READ(PyUnicode_KIND(obj), PyUnicode_DATA(obj), pos)
#define GET_MATCH_CHAR(obj, pos) PyUnicode_READ(PyUnicode_KIND(obj), PyUnicode_DATA(obj), pos)
#else  
#define GET_TEXT_CHAR(obj, pos) ((unsigned char*)PyBytes_AS_STRING(obj))[pos]
#define GET_MATCH_CHAR(obj, pos) ((unsigned char*)PyBytes_AS_STRING(obj))[pos]
#endif

/* Only convert to string for commands that actually expect string arguments */
{
	TE_CHAR *m = NULL;
	if (command != MATCH_ALLINCHARSET && command != MATCH_ISINCHARSET)
	{
		m = TE_STRING_AS_STRING(match);
		if (m == NULL)
		{
			childReturnCode = ERROR_CODE;
			errorType = PyExc_TypeError;
			errorMessage = PyString_FromFormat(
				"Low-level command (%i) argument in entry %d couldn't be converted to a %s object, is a %.50s",
				command,
				(unsigned int)index,
				TE_TABLETYPE == MXTAGTABLE_UNICODETYPE ? "unicode" : "bytes",
				Py_TYPE(match)->tp_name);
		}
	}

	if (childReturnCode == NULL_CODE)
	{
		switch (command)
		{

		case MATCH_ALLIN:

		{
			register Py_ssize_t ml = TE_STRING_GET_SIZE(match);

			DPRINTF("\nAllIn (modern) :\n"
					" looking for match in text\n");

#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
			/* Unicode character-level comparison */
			if (ml > 1)
			{
				for (; childPosition < sliceright; childPosition++)
				{
					register Py_ssize_t j;
					Py_UCS4 ctx = GET_TEXT_CHAR(textobj, childPosition);
					for (j = 0; j < ml; j++) {
						Py_UCS4 mch = GET_MATCH_CHAR(match, j);
						if (ctx == mch) break;
					}
					if (j == ml)
						break;
				}
			}
			else if (ml == 1)
			{
				/* one char only: use faster variant: */
				Py_UCS4 match_char = GET_MATCH_CHAR(match, 0);
				for (; childPosition < sliceright; childPosition++) {
					if (GET_TEXT_CHAR(textobj, childPosition) != match_char)
						break;
				}
			}
#else
			/* Bytes mode */
			if (table->is_multibyte)
			{
				/* UTF-8 mode: decode UTF-8 from text and match, compare codepoints */
				while (childPosition < sliceright)
				{
					Py_UCS4 text_codepoint;
					int text_utf8_len = te_utf8_decode(
						(const unsigned char *)&text[childPosition],
						sliceright - childPosition,
						&text_codepoint);

					if (text_utf8_len <= 0)
						break;  /* Invalid UTF-8 - stop matching */

					/* Search for this codepoint in the match string */
					Py_ssize_t match_pos = 0;
					int found = 0;
					while (match_pos < ml)
					{
						Py_UCS4 match_codepoint;
						int match_utf8_len = te_utf8_decode(
							(const unsigned char *)&m[match_pos],
							ml - match_pos,
							&match_codepoint);

						if (match_utf8_len <= 0)
							break;  /* Invalid UTF-8 in match - stop */

						if (text_codepoint == match_codepoint)
						{
							found = 1;
							break;
						}
						match_pos += match_utf8_len;
					}

					if (!found)
						break;  /* Codepoint not in match set */

					childPosition += text_utf8_len;
				}
			}
			else
			{
				/* Single-byte mode - use original pointer arithmetic */
				register TE_CHAR *tx = &text[childPosition];
				if (ml > 1)
				{
					for (; childPosition < sliceright; tx++, childPosition++)
					{
						register Py_ssize_t j;
						register TE_CHAR *mj = m;
						register TE_CHAR ctx = *tx;
						for (j = 0; j < ml && ctx != *mj; mj++, j++)
							;
						if (j == ml)
							break;
					}
				}
				else if (ml == 1)
				{
					/* one char only: use faster variant: */
					for (; childPosition < sliceright && *tx == *m; tx++, childPosition++)
						;
				}
			}
#endif
			break;
		}

		case MATCH_ALLNOTIN:

		{
			register Py_ssize_t ml = TE_STRING_GET_SIZE(match);

			DPRINTF("\nAllNotIn (modern) :\n"
					" looking for characters not in match\n");

#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
			/* Unicode character-level comparison */
			if (ml > 1)
			{
				for (; childPosition < sliceright; childPosition++)
				{
					register Py_ssize_t j;
					Py_UCS4 ctx = GET_TEXT_CHAR(textobj, childPosition);
					for (j = 0; j < ml; j++) {
						Py_UCS4 mch = GET_MATCH_CHAR(match, j);
						if (ctx == mch) break;
					}
					if (j != ml)
						break;
				}
			}
			else if (ml == 1)
			{
				/* one char only: use faster variant: */
				Py_UCS4 match_char = GET_MATCH_CHAR(match, 0);
				for (; childPosition < sliceright; childPosition++) {
					if (GET_TEXT_CHAR(textobj, childPosition) == match_char)
						break;
				}
			}
#else
			/* Bytes mode */
			if (table->is_multibyte)
			{
				/* UTF-8 mode: decode UTF-8 from text and match, compare codepoints */
				while (childPosition < sliceright)
				{
					Py_UCS4 text_codepoint;
					int text_utf8_len = te_utf8_decode(
						(const unsigned char *)&text[childPosition],
						sliceright - childPosition,
						&text_codepoint);

					if (text_utf8_len <= 0)
						break;  /* Invalid UTF-8 - stop matching */

					/* Search for this codepoint in the match string */
					Py_ssize_t match_pos = 0;
					int found = 0;
					while (match_pos < ml)
					{
						Py_UCS4 match_codepoint;
						int match_utf8_len = te_utf8_decode(
							(const unsigned char *)&m[match_pos],
							ml - match_pos,
							&match_codepoint);

						if (match_utf8_len <= 0)
							break;  /* Invalid UTF-8 in match - stop */

						if (text_codepoint == match_codepoint)
						{
							found = 1;
							break;
						}
						match_pos += match_utf8_len;
					}

					if (found)
						break;  /* Codepoint IS in match set - stop */

					childPosition += text_utf8_len;
				}
			}
			else
			{
				/* Single-byte mode - use original pointer arithmetic */
				register TE_CHAR *tx = &text[childPosition];
				if (ml > 1)
				{
					for (; childPosition < sliceright; tx++, childPosition++)
					{
						register Py_ssize_t j;
						register TE_CHAR *mj = m;
						register TE_CHAR ctx = *tx;
						for (j = 0; j < ml && ctx != *mj; mj++, j++)
							;
						if (j != ml)
							break;
					}
				}
				else if (ml == 1)
				{
					/* one char only: use faster variant: */
					for (; childPosition < sliceright && *tx != *m; tx++, childPosition++)
						;
				}
			}
#endif
			break;
		}

		case MATCH_IS:

		{
			DPRINTF("\nIs (modern) :\n"
					" looking for single character match\n");

#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
			/* Unicode character-level comparison */
			if (childPosition < sliceright &&
				GET_TEXT_CHAR(textobj, childPosition) == GET_MATCH_CHAR(match, 0))
			{
				childPosition++;
			}
#else
			/* Bytes mode - use original pointer arithmetic */
			if (childPosition < sliceright && *(&text[childPosition]) == *m)
			{
				childPosition++;
			}
#endif
			break;
		}

		case MATCH_ISIN:

		{
			register Py_ssize_t ml = TE_STRING_GET_SIZE(match);

			DPRINTF("\nIsIn (modern) :\n"
					" looking for character in match set\n");

#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
			/* Unicode character-level comparison */
			if (ml > 0 && childPosition < sliceright)
			{
				register Py_ssize_t j;
				Py_UCS4 ctx = GET_TEXT_CHAR(textobj, childPosition);
				for (j = 0; j < ml; j++) {
					if (ctx == GET_MATCH_CHAR(match, j))
						break;
				}
				if (j != ml)
					childPosition++;
			}
#else
			/* Bytes mode */
			if (table->is_multibyte && ml > 0 && childPosition < sliceright)
			{
				/* UTF-8 mode: decode UTF-8 from text and match, compare codepoints */
				Py_UCS4 text_codepoint;
				int text_utf8_len = te_utf8_decode(
					(const unsigned char *)&text[childPosition],
					sliceright - childPosition,
					&text_codepoint);

				if (text_utf8_len > 0)
				{
					/* Search for this codepoint in the match string */
					Py_ssize_t match_pos = 0;
					int found = 0;
					while (match_pos < ml)
					{
						Py_UCS4 match_codepoint;
						int match_utf8_len = te_utf8_decode(
							(const unsigned char *)&m[match_pos],
							ml - match_pos,
							&match_codepoint);

						if (match_utf8_len <= 0)
							break;

						if (text_codepoint == match_codepoint)
						{
							found = 1;
							break;
						}
						match_pos += match_utf8_len;
					}

					if (found)
						childPosition += text_utf8_len;
				}
			}
			else if (ml > 0 && childPosition < sliceright)
			{
				/* Single-byte mode - use original pointer arithmetic */
				register TE_CHAR ctx = text[childPosition];
				register Py_ssize_t j;
				register TE_CHAR *mj = m;
				for (j = 0; j < ml && ctx != *mj; mj++, j++)
					;
				if (j != ml)
					childPosition++;
			}
#endif

			break;
		}

		case MATCH_ISNOTIN:

		{
			register Py_ssize_t ml = TE_STRING_GET_SIZE(match);

			DPRINTF("\nIsNotIn (modern) :\n"
					" looking for character not in match set\n");

#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
			/* Unicode character-level comparison */
			if (ml > 0 && childPosition < sliceright)
			{
				register Py_ssize_t j;
				Py_UCS4 ctx = GET_TEXT_CHAR(textobj, childPosition);
				for (j = 0; j < ml; j++) {
					if (ctx == GET_MATCH_CHAR(match, j))
						break;
				}
				if (j == ml)
					childPosition++;
			}
			else
				childPosition++;
#else
			/* Bytes mode */
			if (table->is_multibyte && childPosition < sliceright)
			{
				/* UTF-8 mode: decode UTF-8 from text and match, compare codepoints */
				Py_UCS4 text_codepoint;
				int text_utf8_len = te_utf8_decode(
					(const unsigned char *)&text[childPosition],
					sliceright - childPosition,
					&text_codepoint);

				if (text_utf8_len > 0)
				{
					if (ml > 0)
					{
						/* Search for this codepoint in the match string */
						Py_ssize_t match_pos = 0;
						int found = 0;
						while (match_pos < ml)
						{
							Py_UCS4 match_codepoint;
							int match_utf8_len = te_utf8_decode(
								(const unsigned char *)&m[match_pos],
								ml - match_pos,
								&match_codepoint);

							if (match_utf8_len <= 0)
								break;

							if (text_codepoint == match_codepoint)
							{
								found = 1;
								break;
							}
							match_pos += match_utf8_len;
						}

						if (!found)
							childPosition += text_utf8_len;
					}
					else
					{
						/* Empty match string - everything matches "not in" */
						childPosition += text_utf8_len;
					}
				}
			}
			else if (ml > 0 && childPosition < sliceright)
			{
				/* Single-byte mode - use original pointer arithmetic */
				register TE_CHAR ctx = text[childPosition];
				register Py_ssize_t j;
				register TE_CHAR *mj = m;
				for (j = 0; j < ml && ctx != *mj; mj++, j++)
					;
				if (j == ml)
					childPosition++;
			}
			else
				childPosition++;
#endif

			break;
		}

		case MATCH_WORD:

		{
			Py_ssize_t ml1 = TE_STRING_GET_SIZE(match) - 1;

			DPRINTF("\nWord (modern) :\n"
					" looking for exact word match\n");

			if (childPosition + ml1 >= sliceright)
				break;

#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
			/* Unicode character-level comparison - compare from right to left */
			{
				register Py_ssize_t j = ml1;
				for (; j >= 0; j--) {
					if (GET_TEXT_CHAR(textobj, childPosition + j) != GET_MATCH_CHAR(match, j))
						break;
				}
				if (j >= 0)						   /* not matched */
					childPosition = startPosition; /* reset */
				else
					childPosition += ml1 + 1;
			}
#else
			/* Bytes mode - use original pointer arithmetic */
			{
				register TE_CHAR *tx = &text[childPosition + ml1];
				register Py_ssize_t j = ml1;
				register TE_CHAR *mj = &m[j];

				/* compare from right to left */
				for (; j >= 0 && *tx == *mj;
					 tx--, mj--, j--)
					;

				if (j >= 0)						   /* not matched */
					childPosition = startPosition; /* reset */
				else
					childPosition += ml1 + 1;
			}
#endif
			break;
		}

		case MATCH_WORDSTART:
		case MATCH_WORDEND:

		{
			Py_ssize_t ml1 = TE_STRING_GET_SIZE(match) - 1;

			DPRINTF("\nWordStart/End (modern) :\n"
					" searching for word in text\n");

			if (ml1 >= 0)
			{
#if (TE_TABLETYPE == MXTAGTABLE_UNICODETYPE)
				/* Unicode character-level comparison - brute-force search */
				for (;;)
				{
					register Py_ssize_t j = ml1;

					if (childPosition + j >= sliceright)
					{
						/* reached eof: no match, rewind */
						childPosition = startPosition;
						break;
					}

					/* scan from right to left */
					for (; j >= 0; j--) {
						if (GET_TEXT_CHAR(textobj, childPosition + j) != GET_MATCH_CHAR(match, j))
							break;
					}

					if (j < 0)
					{
						/* found */
						if (command == MATCH_WORDEND)
							childPosition += ml1 + 1;
						break;
					}
					/* not found: advance one char */
					childPosition++;
				}
#else
				/* Bytes mode - use original pointer arithmetic */
				register TE_CHAR *tx = &text[childPosition];

				/* Brute-force method; from right to left */
				for (;;)
				{
					register Py_ssize_t j = ml1;
					register TE_CHAR *mj = &m[j];

					if (childPosition + j >= sliceright)
					{
						/* reached eof: no match, rewind */
						childPosition = startPosition;
						break;
					}

					/* scan from right to left */
					for (tx += j; j >= 0 && *tx == *mj;
						 tx--, mj--, j--)
						;

					if (j < 0)
					{
						/* found */
						if (command == MATCH_WORDEND)
							childPosition += ml1 + 1;
						break;
					}
					/* not found: rewind and advance one char */
					tx -= j - 1;
					childPosition++;
				}
#endif
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
					" in string     = '%.40s'\n",
					(long)match, tx);

			for (;
				 childPosition < sliceright &&
				 (m[((unsigned char)*tx) >> 3] &
				  (1 << (*tx & 7))) > 0;
				 tx++, childPosition++)
				;

			break;
		}

		case MATCH_ISINSET:

		{
			register TE_CHAR *tx = &text[childPosition];
			unsigned char *m = (unsigned char *)PyString_AS_STRING(match);

			DPRINTF("\nIsInSet :\n"
					" looking for   = set at 0x%lx\n"
					" in string     = '%.40s'\n",
					(long)match, tx);

			if (childPosition < sliceright &&
				(m[((unsigned char)*tx) >> 3] &
				 (1 << (*tx & 7))) > 0)
				childPosition++;

			break;
		}

#endif

		case MATCH_ALLINCHARSET:

		{
			DPRINTF("\nAllInCharSet :\n"
					" looking for   = CharSet at 0x%lx\n"
					" in string     = '%.40s'\n",
					(long)match, &text[childPosition]);

#if (TE_TABLETYPE == MXTAGTABLE_STRINGTYPE)
			/* Check if we're in UTF-8 encoded mode */
			if (table->is_multibyte)
			{
				/* UTF-8 mode: decode UTF-8 sequences one at a time and test each codepoint */
				while (childPosition < sliceright)
				{
					Py_UCS4 codepoint;
					int utf8_len = te_utf8_decode(
						(const unsigned char *)&text[childPosition],
						sliceright - childPosition,
						&codepoint);

					if (utf8_len <= 0)
					{
						/* Invalid UTF-8 sequence - stop matching */
						break;
					}

					int test = mxCharSet_ContainsUnicodeChar(match, codepoint);
					if (test < 0)
					{
						childReturnCode = ERROR_CODE;
						errorType = PyExc_SystemError;
						errorMessage = PyString_FromFormat(
							"Character set match returned value < 0 (%d): probable bug in text processing engine",
							test);
						break;
					}
					else if (test)
					{
						childPosition += utf8_len;  /* Advance by UTF-8 sequence length */
					}
					else
					{
						/* Character not in set - stop matching */
						break;
					}
				}
			}
			else
			{
				/* Single-byte mode: use original mxCharSet_Match */
				Py_ssize_t matching = mxCharSet_Match(match,
													  textobj,
													  childPosition,
													  sliceright,
													  1);
				if (matching < 0)
				{
					childReturnCode = ERROR_CODE;
					errorType = PyExc_SystemError;
					errorMessage = PyString_FromFormat(
						"Character set match returned value < 0 (%d): probable bug in text processing engine",
						(unsigned int)matching);
				}
				else
				{
					childPosition += matching;
				}
			}
#else
			/* Unicode mode: use original mxCharSet_Match */
			{
				Py_ssize_t matching = mxCharSet_Match(match,
													  textobj,
													  childPosition,
													  sliceright,
													  1);
				if (matching < 0)
				{
					childReturnCode = ERROR_CODE;
					errorType = PyExc_SystemError;
					errorMessage = PyString_FromFormat(
						"Character set match returned value < 0 (%d): probable bug in text processing engine",
						(unsigned int)matching);
				}
				else
				{
					childPosition += matching;
				}
			}
#endif
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
			/* Check if we're in UTF-8 encoded mode */
			if (table->is_multibyte && childPosition < sliceright)
			{
				/* UTF-8 mode: decode the UTF-8 sequence and test the codepoint */
				Py_UCS4 codepoint;
				int utf8_len = te_utf8_decode(
					(const unsigned char *)&text[childPosition],
					sliceright - childPosition,
					&codepoint);

				if (utf8_len > 0)
				{
					test = mxCharSet_ContainsUnicodeChar(match, codepoint);
					if (test < 0)
					{
						childReturnCode = ERROR_CODE;
						errorType = PyExc_SystemError;
						errorMessage = PyString_FromFormat(
							"Character set match returned value < 0 (%i): probable bug in text processing engine",
							test);
					}
					else if (test)
					{
						childPosition += utf8_len;  /* Advance by UTF-8 sequence length */
					}
				}
				/* If utf8_len == 0, invalid UTF-8 - no match, childPosition unchanged */
			}
			else
			{
				/* Single-byte mode: test byte directly */
				test = mxCharSet_ContainsChar(match, text[childPosition]);
				if (test < 0)
				{
					childReturnCode = ERROR_CODE;
					errorType = PyExc_SystemError;
					errorMessage = PyString_FromFormat(
						"Character set match returned value < 0 (%i): probable bug in text processing engine",
						test);
				}
				else if (test)
				{
					childPosition++;
				}
			}
#else
			test = mxCharSet_ContainsUnicodeChar(match, text[childPosition]);
			if (test < 0)
			{
				childReturnCode = ERROR_CODE;
				errorType = PyExc_SystemError;
				errorMessage = PyString_FromFormat(
					"Character set match returned value < 0 (%i): probable bug in text processing engine",
					test);
			}
			else if (test)
			{
				childPosition++;
			}
#endif
			break;
		}
		default:
		{
			childReturnCode = ERROR_CODE;
			errorType = PyExc_ValueError;
			errorMessage = PyString_FromFormat(
				"Unrecognised Low-Level command code %i, maximum low-level code is %i",
				command,
				MATCH_MAX_LOWLEVEL);
		}
			/* end of the switch, this child is finished */
		}
	}

	/* simple determination for these commands (hence calling them low-level) */
	if (childReturnCode == NULL_CODE)
	{
		if (childPosition > childStart)
		{
			childReturnCode = SUCCESS_CODE;
		}
		else
		{
			childReturnCode = FAILURE_CODE;
		}
	}
}
