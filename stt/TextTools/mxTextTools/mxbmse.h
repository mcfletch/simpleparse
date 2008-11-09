#ifndef MXBMSE_H
#define MXBMSE_H
/* 
  mxbmse -- Fast Boyer Moore Search Algorithm (Version 0.8)

  The implementation is reentrant and thread safe. While the
  general idea behind the Boyer Moore algorithm are in the public
  domain, this implementation falls under the following copyright:

  Copyright (c) 1997-2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
  Copyright (c) 2000-2002, eGenix.com Software GmbH; mailto:info@egenix.com

                        All Rights Reserved

  See the documentation for copying information or contact the author
  (mal@lemburg.com).

*/

#ifdef __cplusplus
extern "C" {
#endif

/* --- Fast Boyer-Moore Implementation (8-bit) ---------------------------- */

/* sanity check switches */
/*#define SAFER 1*/

/* SHIFT must have enough bits to store len(match) 
   - using 'char' here makes the routines run 15% slower than
     with 'int', on the other hand, 'int' is at least 4 times
     larger than 'char'
*/
#ifndef BM_SHIFT_TYPE
# define BM_SHIFT_TYPE int
#endif

typedef struct {
    char *match;
    int match_len;
    char *eom;
    char *pt;
    BM_SHIFT_TYPE shift[256]; /* char-based shift table */
} mxbmse_data;

extern mxbmse_data *bm_init(char *match,
			    int match_len);
extern void bm_free(mxbmse_data *c);
extern int bm_search(mxbmse_data *c,
		     char *text,
		     int start,
		     int stop);
extern int bm_tr_search(mxbmse_data *c,
			char *text,
			int start,
			int stop,
			char *tr);

#define BM_MATCH_LEN(bm) ((mxbmse_data *)bm)->match_len

/* EOF */
#ifdef __cplusplus
}
#endif
#endif
