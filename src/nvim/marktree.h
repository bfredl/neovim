#ifndef NVIM_MARKTREE_H
#define NVIM_MARKTREE_H

#include <stdint.h>
#include "nvim/map.h"
#include "nvim/garray.h"

#define MT_MAX_DEPTH 20
#define MT_BRANCH_FACTOR 10

typedef struct {
  int32_t row;
  int32_t col;
} mtpos_t;

typedef struct {
  mtpos_t pos;
  uint64_t id;
  bool right_gravity;
} mtmark_t;

typedef struct mttree_s MarkTree;
typedef struct mtnode_s mtnode_t;

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
