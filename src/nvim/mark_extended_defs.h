#ifndef NVIM_MARK_EXTENDED_DEFS_H
#define NVIM_MARK_EXTENDED_DEFS_H

#include "nvim/pos.h"  // for colnr_T
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"

struct ExtmarkLine;

typedef struct Extmark
{
  uint64_t ns_id;
  uint64_t mark_id;
  struct ExtmarkLine *line;
  colnr_T col;
} Extmark;

typedef struct
{
  uint64_t ns_id;
  uint64_t mark_id;
  // TODO(bfredl): can has ext-ranges?
  // uint64_t pos2;
  // TODO(bufhl): annotation should be a "sub-item"
} ExtmarkItem;




typedef struct undo_object ExtmarkUndoObject;
typedef kvec_t(ExtmarkUndoObject) extmark_undo_vec_t;


#endif  // NVIM_MARK_EXTENDED_DEFS_H
