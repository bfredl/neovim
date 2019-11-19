#ifndef NVIM_MARKTREE_H
#define NVIM_MARKTREE_H

#include <stdint.h>
#include "nvim/map.h"
#include "nvim/garray.h"

#define MT_MAX_DEPTH 64
#define MT_BRANCH_FACTOR 10

typedef struct {
  int32_t row;
  int32_t col;
} mtpos_t;

// NB actual marks MUST have id > 0, so we can use (row,col,0) pseudo-key for
// "space before (row,col)"
typedef struct {
  mtpos_t pos;
  uint64_t id;
} mtkey_t;

typedef struct mttree_s MarkTree;
typedef struct mtnode_s mtnode_t;

typedef struct {
  // TODO(bfredl): really redundant with "backlinks", but keep now so we can do
  // consistency check easily.
  mtnode_t *x;
  int i;
} mtfailpos_t;

typedef struct {
  mtpos_t pos;
  mtfailpos_t stack[MT_MAX_DEPTH], *p;
} MarkTreeIterFail;

typedef struct {
  int oldcol;
  int i;
} iterstate_t;

typedef struct {
  mtpos_t pos;
  int lvl;
  mtnode_t *node;
  int i;
  iterstate_t s[MT_MAX_DEPTH];
} MarkTreeIter;


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "marktree.h.generated.h"
#endif


#endif  // NVIM_MARKTREE_H
