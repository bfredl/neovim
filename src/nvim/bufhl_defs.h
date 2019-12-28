#ifndef NVIM_BUFHL_DEFS_H
#define NVIM_BUFHL_DEFS_H

#include "nvim/pos.h"
#include "nvim/marktree.h"
#include "nvim/lib/kvec.h"
#include "nvim/lib/kbtree.h"

// bufhl: buffer specific highlighting

typedef struct {
  char *text;
  int hl_id;
} VirtTextChunk;

typedef kvec_t(VirtTextChunk) VirtText;

typedef struct {
  int src_id;
  int hl_id;  // highlight group
  VirtText virt_text;
} BufhlItem;


typedef struct {
  MarkTreeIter itr[1];
  int current;
  colnr_T valid_to;
  int row;
  // TODO: share the buffer?
  kvec_t(ssize_t) active;
  VirtText *virt_text;
} BufhlLineInfo;

#endif  // NVIM_BUFHL_DEFS_H
