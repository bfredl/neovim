#ifndef NVIM_EXTMARK_H
#define NVIM_EXTMARK_H

/* #include "nvim/ex_cmds_defs.h" // for exarg_T */
#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"
#include "nvim/map.h"

typedef struct ExtendedMark ExtendedMark;
struct ExtendedMark {
  char *name;
  fmark_T fmark;
  ExtendedMark *prev;
};

#define extmark_pos_cmp(a, b) (pos_cmp((a).fmark.mark, (b).fmark.mark))

#define FOR_ALL_EXTMARKS(buf) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  if (buf->b_extmarks_tree) { \
      kb_itr_first(extmarks, buf->b_extmarks_tree, &itr);\
      for (; kb_itr_valid(&itr) \
           ; kb_itr_next(extmarks, buf->b_extmarks_tree, &itr)){\
               extmark = &kb_itr_key(ExtendedMark, &itr);

#define END_LOOP }}

typedef kvec_t(cstr_t) ExtmarkNames;
typedef Map(cstr_t, ptr_t) ExtendedMarkPtr;
KBTREE_INIT(extmarks, ExtendedMark, extmark_pos_cmp)

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_EXTMARK_H
