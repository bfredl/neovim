#ifndef NVIM_EX_CMDS_H
#define NVIM_EX_CMDS_H

#include <stdbool.h>

#include "nvim/os/time.h"
#include "nvim/eval_defs.h"
#include "nvim/pos.h"
#include "nvim/lib/klist.h"

/* flags for do_ecmd() */
#define ECMD_HIDE       0x01    /* don't free the current buffer */
#define ECMD_SET_HELP   0x02    /* set b_help flag of (new) buffer before
                                   opening file */
#define ECMD_OLDBUF     0x04    /* use existing buffer if it exists */
#define ECMD_FORCEIT    0x08    /* ! used in Ex command */
#define ECMD_ADDBUF     0x10    /* don't edit, just add to buffer list */

/* for lnum argument in do_ecmd() */
#define ECMD_LASTL      (linenr_T)0     /* use last position in loaded file */
#define ECMD_LAST       (linenr_T)-1    /* use last position in all files */
#define ECMD_ONE        (linenr_T)1     /* use first line */

/// Previous :substitute replacement string definition
typedef struct {
  char *sub;            ///< Previous replacement string.
  Timestamp timestamp;  ///< Time when it was last set.
  list_T *additional_elements;  ///< Additional data left from ShaDa file.
} SubReplacementString;


// Defs for inc_sub functionality

// List of matches in a line
#define _noop(x)
KLIST_INIT(colnr_T, colnr_T, _noop)
typedef klist_t(colnr_T) klist_colnr_T;
#undef _noop

/// Structure to backup and display matched lines in incsubstitution
typedef struct {
  linenr_T lnum;
  long nmatch;
  char_u *line;
  klist_colnr_T *start_col;
} MatchedLine;

// List of matched lines
#define _dealloc_MatchedLine(x) \
do { \
  if (x->data.line) { xfree(x->data.line); } \
  if (x->data.start_col) { kl_destroy(colnr_T, x->data.start_col); } \
  } while (0)

KLIST_INIT(MatchedLine, MatchedLine, _dealloc_MatchedLine)
typedef klist_t(MatchedLine) klist_MatchedLine;
#undef _dealloc_MatchedLine

// End defs for inc_sub functionality


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "ex_cmds.h.generated.h"
#endif
#endif  // NVIM_EX_CMDS_H
