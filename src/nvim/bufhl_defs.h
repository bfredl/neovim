#ifndef NVIM_BUFHL_DEFS_H
#define NVIM_BUFHL_DEFS_H

#include "nvim/pos.h"
#include "nvim/marktree.h"
#include "nvim/lib/kvec.h"
#include "nvim/lib/kbtree.h"

// bufhl: buffer specific highlighting

typedef struct {
  int src_id;
  int hl_id;  // highlight group
  uint64_t start;  // mark to start highlight
  uint64_t stop;  // mark to end highlight
} BufhlItem;

typedef struct {
  char *text;
  int hl_id;
} VirtTextChunk;

typedef kvec_t(VirtTextChunk) VirtText;

typedef struct {
  linenr_T line;
  kvec_t(BufhlItem) items;
  int virt_text_src;
  VirtText virt_text;
} BufhlLine;
#define BUFHLLINE_INIT(l) { l, KV_INITIAL_VALUE, 0,  KV_INITIAL_VALUE }

typedef struct {
  BufhlLine *line;
  MarkTreeIter itr[1];
  int current;
  colnr_T valid_to;
  int row;
  // TODO: share the buffer?
  kvec_t(size_t) active;
} BufhlLineInfo;

#define BUFHL_CMP(a, b) ((int)(((a)->line - (b)->line)))
KBTREE_INIT(bufhl, BufhlLine *, BUFHL_CMP, 10)  // -V512
typedef kbtree_t(bufhl) BufhlInfo;
#endif  // NVIM_BUFHL_DEFS_H
