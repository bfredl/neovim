#ifndef NVIM_MARK_EXTENDED_DEFS_H
#define NVIM_MARK_EXTENDED_DEFS_H

#include "nvim/pos.h"  // for colnr_T
#include "nvim/lib/kvec.h"

typedef struct {
  char *text;
  int hl_id;
} VirtTextChunk;

typedef kvec_t(VirtTextChunk) VirtText;

typedef struct
{
  uint64_t ns_id;
  uint64_t mark_id;
  int hl_id;  // highlight group
  // TODO(bfredl): virt_text is pretty larger than the rest,
  // pointer indirection?
  VirtText virt_text;
} ExtmarkItem;

typedef struct undo_object ExtmarkUndoObject;
typedef kvec_t(ExtmarkUndoObject) extmark_undo_vec_t;

#endif  // NVIM_MARK_EXTENDED_DEFS_H
