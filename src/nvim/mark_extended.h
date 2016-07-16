#ifndef NVIM_EXTMARK_H
#define NVIM_EXTMARK_H

/* #include "nvim/ex_cmds_defs.h" // for exarg_T */
#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"

#define EXTMARK_MAXLEN 24
#define extmark_pos_cmp(a, b) (pos_cmp((a).fmark.mark, (b).fmark.mark))

typedef struct ExtendedMark ExtendedMark;
struct ExtendedMark {
  fmark_T fmark;
  ExtendedMark *next;
  ExtendedMark *prev;
};

KBTREE_INIT(extmarks, ExtendedMark, extmark_pos_cmp)

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_EXTMARK_H
