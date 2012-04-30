/* 
  mxbmse -- Fast Boyer Moore Search Algorithm (Version 0.9)

  The implementation is reentrant and thread safe. While the
  general ideas behind the Boyer Moore algorithm are in the public
  domain, this implementation falls under the following copyright:

  Copyright (c) 1997-2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com

                        All Rights Reserved

  See the documentation for copying information or contact the author
  (mal@lemburg.com).
*/

/* to turn on the debugging printfs (DPRINTF):*/
/* #define MAL_DEBUG */

/* Logging file used by debugging facility */
#ifndef MAL_DEBUG_OUTPUTFILE
# define MAL_DEBUG_OUTPUTFILE "mxTextSearch.log"
#endif

#ifdef MAL_DEBUG_WITH_PYTHON
# include "mx.h"
#endif

#include "mxstdlib.h"
#include "mxbmse.h"

/* --- Fast Boyer-Moore Implementation (8-bit) ---------------------------- */

mxbmse_data *bm_init(char *match,
		     int match_len)
{
    mxbmse_data *c;
    int i;
    BM_SHIFT_TYPE *shift;
    char *m;

    c = newstruct(mxbmse_data);
    c->match = match;
    c->match_len = match_len;
    c->eom = match + match_len - 1;

    /* Length 1 matching does not use a shift table */
    if (match_len == 1)
	return c;

    /* Init shift table */
    for ( shift = c->shift, i = 256; i > 0; i--, shift++ )
	*shift = (BM_SHIFT_TYPE) match_len;

    DPRINTF("shift table for match='%s'\n",match);
    for ( shift = c->shift, m = match, i = match_len - 1; 
	  i >= 0; 
	  i--, m++ ) {
	shift[ (unsigned char) *m ] = (BM_SHIFT_TYPE) i;
	DPRINTF("  char = '%c'  shift = %i\n", *m, i);
    }

    return c;
}

void bm_free(mxbmse_data *c)
{
    if (c)
	free(c);
}

int bm_search(mxbmse_data *c,
	      char *text,
	      int start,
	      int text_len)
{
    register char *pt;
    register char *eot = text + text_len;
    
    /* Error check */
    if (c == NULL) 
	return -1;

    /* Init text pointer */
    pt = text + start + c->match_len - 1;

    DPRINTF("Init :  %2i %20.20s \t text: %2i %20.20s\n",
	    c->match_len,c->match,start,text+start);

    if (c->match_len > 1)
	for (;;) {
	    register char *pm;

	    pm = c->eom;

	    for (;pt < eot && *pt != *pm; 
		  pt += c->shift[(unsigned char) *pt]);

	    if (pt >= eot) 
		break;

	    /* First char matches.. what about the others ? */
	    {
		register int im = c->match_len;

		do {
		    DPRINTF("=match: %2i '%20.20s' \t text: '%20.20s'\n",
			    im,pm,pt);
		    if (--im == 0) 
			/* Match */
			return pt - text + c->match_len;
		    pt--;
		    pm--;
		} while (*pt == *pm);

		/* Mismatch after match: use shift-table */
		{
		    register int a,b;

		    a = c->shift[(unsigned char) *pt];
		    b = c->match_len - im + 1;
		    DPRINTF("!match: %2i '%20.20s' \t text: '%20.20s' "
			    "(sh=%i)\n",
			    im,pm,pt,max(a,b));
		    pt += (a > b) ? a : b;
		}
	    }

	}

    /* Special case: matching string has length 1 */
    else {
	register char m = *c->eom;
	
	for (;pt < eot; pt++)
	    if (*pt == m)
		/* Match */
		return pt - text + 1;
    }

    return start; /* no match */
}

/* bm search using the translate table -- 45% slower */

int bm_tr_search(mxbmse_data *c,
		 char *text,
		 int start,
		 int text_len,
		 char *tr)
{
    register char *pt;
    register char *eot = text + text_len;

    /* Error check */
    if (c == NULL) 
	return -1;

    /* Init text pointer */
    pt = text + start + c->match_len - 1;

    DPRINTF("Init :  %2i '%20.20s' \t text: %2i '%20.20s'\n",
	    c->match_len,c->match,start,text+start);

    if (c->match_len > 1)
	for (;;) {
	    register char *pm;

	    pm = c->eom;

	    for (;pt < eot && tr[(unsigned char) *pt] != *pm; 
		 pt += c->shift[(unsigned char) tr[(unsigned char) *pt]]);

	    if (pt >= eot) 
		break;

	    /* First char matches.. what about the others ? */
	    {
		register int im = c->match_len;

		do {
		    DPRINTF("=match: %2i '%20.20s' \t text: '%20.20s'\n",
			    im,pm,pt);
		    if (--im == 0) 
			/* Match */
			return pt - text + c->match_len;
		    pt--;
		    pm--;
		} while (tr[(unsigned char) *pt] == *pm);

		/* Mismatch after match: use shift-table */
		{
		    register int a,b;

		    a = c->shift[(unsigned char) tr[(unsigned char) *pt]];
		    b = c->match_len - im + 1;
		    DPRINTF("!match: %2i '%20.20s' \t text: '%20.20s' "
			    "(sh=%i)\n",
			    im,pm,pt,max(a,b));
		    pt += (a > b)?a:b;
		}
	    }

	}

    /* Special case: matching string has length 1 */
    else {
	register char m = *c->eom;
	
	for (;pt < eot; pt++)
	    if (*pt == m)
		/* Match */
		return pt - text + 1;
    }

    return start; /* no match */
}

