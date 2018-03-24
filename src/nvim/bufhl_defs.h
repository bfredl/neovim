#ifndef NVIM_BUFHL_DEFS_H
#define NVIM_BUFHL_DEFS_H

#include "nvim/pos.h"
#include "nvim/lib/kvec.h"
#include "nvim/lib/kbtree.h"

// bufhl: buffer specific highlighting

typedef struct {
  int src_id;
  int hl_id;  // highlight group
  colnr_T start;  // first column to highlight
  colnr_T stop;  // last column to highlight
} BufhlItem;

typedef kvec_t(BufhlItem) BufhlItemVec;

typedef struct {
  linenr_T line;
  BufhlItemVec items;
  char *eol_text;
  int eol_id;
  int eol_src;
} BufhlLine;
#define BUFHLLINE_INIT(l) { l, KV_INITIAL_VALUE, NULL, 0, 0 }

// TODO: merge me with BufhlLine?
typedef struct {
  BufhlItemVec entries;
  int current;
  colnr_T valid_to;
  char *eol_text;
  int eol_attr;
} BufhlLineInfo;

#define BUFHL_CMP(a, b) ((int)(((a)->line - (b)->line)))
KBTREE_INIT(bufhl, BufhlLine *, BUFHL_CMP, 10)  // -V512
typedef kbtree_t(bufhl) BufhlInfo;
#endif  // NVIM_BUFHL_DEFS_H
