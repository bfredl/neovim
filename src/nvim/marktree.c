#include <stdint.h>
#include "nvim/map.h"
#define T 10
#define ILEN (sizeof(kbnode_t)+(2*T)*sizeof(void *))

#define MT_MAX_DEPTH 64

typedef struct {
  int32_t row;
  int32_t col;
  uint64_t id;
} key_t;

typedef struct mtnode_s mtnode_t;
struct mtnode_s {
  int32_t n;
  bool is_internal;
  key_t key[2 * T - 1];
  mtnode_t *ptr[];
};

typedef struct {
  mtnode_t *root;
  int n_keys, n_nodes;
  PMap(uint64_t) *id2node;
} mttree_impl_t;

typedef struct {
  kbnode_t *x;
  int i;
} mtpos_t;

typedef struct {
  mtpos_t stack[MT_MAX_DEPTH], *p;
} mtitr_impl_t;

