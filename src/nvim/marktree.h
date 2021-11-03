#ifndef NVIM_MARKTREE_H
#define NVIM_MARKTREE_H

#include <stdint.h>

#include "nvim/garray.h"
#include "nvim/map.h"
#include "nvim/pos.h"

#define MT_MAX_DEPTH 20
#define MT_BRANCH_FACTOR 10

typedef struct {
  int32_t row;
  int32_t col;
} mtpos_t;

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


// Internal storage
//
// NB: actual marks have id > 0, so we can use (row,col,0) pseudo-key for
// "space before (row,col)"
typedef struct {
  mtpos_t pos;
  uint32_t ns;
  uint32_t foo_id; // TODO: putta tillbaka when all id usages checked
  int32_t hl_id;
  uint16_t flags;
  int16_t prio;
} mtkey_t;
#define MT_INVALID_KEY (mtkey_t){ { -1, -1 }, 0, 0, 0, 0, 0 }

#define MARKTREE_END_FLAG (((uint64_t)1) << 63)

#define MT_FLAG_END (((uint16_t)1) << 0)
#define MT_FLAG_PAIRED (((uint16_t)1) << 1)

// These _must_ be last to preserve ordering of marks
#define MT_FLAG_RIGHT_GRAVITY (((uint16_t)1) << 14)
#define MT_FLAG_LAST (((uint16_t)1) << 15)

#define TODO_uint32_t uint32_t

static inline uint64_t mt_lookup_id(uint32_t ns, uint32_t id, bool enda) {
  return (uint64_t)ns << 32 | id | (enda?MARKTREE_END_FLAG:0);
}

static inline uint64_t mt_lookup_key(mtkey_t key) {
  return mt_lookup_id(key.ns, key.foo_id, key.flags & MT_FLAG_END);
}

static inline bool mt_paired(mtkey_t key)
{
  return key.flags & MT_FLAG_PAIRED;
}

static inline bool mt_end(mtkey_t key)
{
  return key.flags & MT_FLAG_END;
}

static inline bool mt_right(mtkey_t key)
{
  return key.flags & MT_FLAG_RIGHT_GRAVITY;
}

struct mtnode_s {
  int32_t n;
  int32_t level;
  // TODO(bfredl): we could consider having a only-sometimes-valid
  // index into parent for faster "cached" lookup.
  mtnode_t *parent;
  mtkey_t key[2 * MT_BRANCH_FACTOR - 1];
  mtnode_t *ptr[];
};

// TODO(bfredl): the iterator is pretty much everpresent, make it part of the
// tree struct itself?
typedef struct {
  mtnode_t *root;
  size_t n_keys, n_nodes;
  // TODO(bfredl): the pointer to node could be part of the larger
  // Map(uint64_t, ExtmarkItem) essentially;
  PMap(uint64_t) id2node[1];
} MarkTree;


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "marktree.h.generated.h"
#endif

#define DECOR_LEVELS 4
#define DECOR_OFFSET 61
#define DECOR_MASK (((uint64_t)(DECOR_LEVELS-1)) << DECOR_OFFSET)

static inline uint8_t marktree_decor_level(mtkey_t key)
{
  //return (uint8_t)((id&DECOR_MASK) >> DECOR_OFFSET);
  return 0;
}

#endif  // NVIM_MARKTREE_H
