#ifndef NVIM_MARKTREE_H
#define NVIM_MARKTREE_H

#include <stdint.h>
#include "nvim/map.h"

#define MT_MAX_DEPTH 64
#define MT_BRANCH_FACTOR 10
#define T MT_BRANCH_FACTOR
// NB actual marks MUST have id > 0, so we can use (row,col,0) pseudo-key for
// "space before (row,col)"
typedef struct {
  int32_t row;
  int32_t col;
  uint64_t id;
} mtkey_t;

typedef struct mtnode_s mtnode_t;
struct mtnode_s {
  int32_t n;
  bool is_internal;
  mtkey_t key[2 * T - 1];
  mtnode_t *parent;
  mtnode_t *ptr[];
};

typedef struct mttree_s {
  mtnode_t *root;
  int n_keys, n_nodes;
  // TODO: the pointer to node could be part of a larger Map(uint64_t, MarkState);
  PMap(uint64_t) id2node;
} MarkTree;

typedef struct {
  mtnode_t *x;
  int i;
} mtpos_t;

typedef struct {
  mtpos_t stack[MT_MAX_DEPTH], *p;
} mtitr_impl_t;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "marktree.h.generated.h"
#endif

#undef T

#endif  // NVIM_MARKTREE_H
