#include "nvim/map.h"

#include "nvim/marktree.h"
#define T MT_BRANCH_FACTOR
#define ILEN (sizeof(mtnode_t)+(2*T)*sizeof(void *))


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "marktree.c.generated.h"
#endif

// DELET, only for luajit test
MarkTree *marktree_new(void) {
  MarkTree *ret = xcalloc(1,sizeof(*ret));
  return ret;
}

#define mt_generic_cmp(a, b) (((b) < (a)) - ((a) < (b)))
static int key_cmp(mtkey_t a, mtkey_t b)
{
  int cmp = mt_generic_cmp(a.row, b.row);
  if (cmp != 0) {
    return cmp;
  }
  cmp = mt_generic_cmp(a.col, b.col);
  if (cmp != 0) {
    return cmp;
  }
  // NB: keeping the events at the same pos sorted by id is actually not
  // necessary only make sure that START is before END etc.
  return mt_generic_cmp(a.id, b.id);
}

static inline int marktree_getp_aux(const mtnode_t *x, mtkey_t  k, int *r)
{
  int tr, *rr, begin = 0, end = x->n;
  if (x->n == 0) {
    return -1;
  }
  rr = r? r : &tr;
  while (begin < end) {
    int mid = (begin + end) >> 1;
    if (key_cmp(x->key[mid], k) < 0) {
      begin = mid + 1;
    } else {
      end = mid;
    }
  }
  if (begin == x->n) { *rr = 1; return x->n - 1; }
  if ((*rr = key_cmp(k, x->key[begin])) < 0) {
    begin--;
  }
  return begin;
}

static inline void refkey(MarkTree *b, mtnode_t *x, int i)
{
  pmap_put(uint64_t)(b->id2node, x->key[i].id, x);
}

// x must be an internal node, which is not full
// x->ptr[i] should be a full node, i e x->ptr[i]->n == 2*T-1
static inline void marktree_split(MarkTree *b, mtnode_t *x, int i)
{
  mtnode_t *y = x->ptr[i];
  mtnode_t *z;
  z = (mtnode_t *)xcalloc(1, y->is_internal? ILEN : sizeof(mtnode_t));
  b->n_nodes++;
  z->is_internal = y->is_internal;
  z->n = T - 1;
  memcpy(z->key, &y->key[T], sizeof(key_t) * (T - 1));
  for (int j = 0; j < T-1; j++) {
    refkey(b, z, j);
  }
  if (y->is_internal) {
    memcpy(z->ptr, &y->ptr[T], sizeof(mtnode_t *) * T);
    for (int j = 0; j < T; j++) {
      z->ptr[i]->parent = z;
    }
  }
  y->n = T - 1;
  memmove(&x->ptr[i + 2], &x->ptr[i + 1],
          sizeof(mtnode_t *) * (size_t)(x->n - i));
  x->ptr[i + 1] = z;
  z->parent = x;  // == y->parent
  memmove(&x->key[i + 1], &x->key[i], sizeof(mtkey_t) * (size_t)(x->n - i));

  // move key to internal layer:
  x->key[i] = y->key[T - 1];
  refkey(b, x, i);
  x->n++;
}

// x must not be a full node (even if there might be internal space)
static inline void marktree_putp_aux(MarkTree *b, mtnode_t *x, mtkey_t k)
{
  int i = x->n - 1;
  if (x->is_internal == 0) {
    i = marktree_getp_aux(x, k, 0);
    if (i != x->n - 1) {
      memmove(&x->key[i + 2], &x->key[i + 1],
              (size_t)(x->n - i - 1) * sizeof(mtkey_t));
    }
    x->key[i + 1] = k;
    refkey(b, x, i+1);
    x->n++;
  } else {
    i = marktree_getp_aux(x, k, 0) + 1;
    if (x->ptr[i]->n == 2 * T - 1) {
      marktree_split(b, x, i);
      if (key_cmp(k, x->key[i]) > 0) {
        i++;
      }
    }
    marktree_putp_aux(b, x->ptr[i], k);
  }
}

void marktree_put(MarkTree *b, mtkey_t k)
{
  if (!b->root) {
    b->root = (mtnode_t*)xcalloc(1, ILEN);
    b->id2node = pmap_new(uint64_t)();
    b->n_nodes++;
  }
  mtnode_t *r, *s;
  b->n_keys++;
  r = b->root;
  if (r->n == 2 * T - 1) {
    b->n_nodes++;
    s = (mtnode_t *)xcalloc(1, ILEN);
    b->root = s; s->is_internal = 1; s->n = 0;
    s->ptr[0] = r;
    r->parent = s;
    marktree_split(b, s, 0);
    r = s;
  }
  marktree_putp_aux(b, r, k);
}


