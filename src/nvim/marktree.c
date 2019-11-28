
#include "nvim/marktree.h"
#include "nvim/garray.h"

#define T MT_BRANCH_FACTOR
#define ILEN (sizeof(mtnode_t)+(2*T)*sizeof(void *))
#define key_t SKRAPET

struct mtnode_s {
  int32_t n;
  int32_t level;
  // TODO(bfredl): we could consider having a only-sometimes-valid
  // index into parent for faster "chached" lookup.
  mtnode_t *parent;
  mtkey_t key[2 * T - 1];
  mtnode_t *ptr[];
};

struct mttree_s {
  mtnode_t *root;
  int n_keys, n_nodes;
  // TODO(bfredl): the pointer to node could be part of a larger Map(uint64_t, MarkState);
  PMap(uint64_t) *id2node;
  bool rel;

};

static bool pos_leq(mtpos_t a, mtpos_t b)
{
  return a.row < b.row || (a.row == b.row && a.col <= b.col);
}

static void relative(mtpos_t base, mtpos_t *val)
{
  assert(pos_leq(base, *val));
  if (val->row == base.row) {
    val->row = 0;
    val->col -= base.col;
  } else {
    val->row -= base.row;
  }
}

static void unrelative(mtpos_t base, mtpos_t *val)
{
  if (val->row == 0) {
    val->row = base.row;
    val->col += base.col;
  } else {
    val->row += base.row;
  }
}

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "marktree.c.generated.h"
#endif

// DELET, only for luajit test
MarkTree *marktree_new(bool rel) {
  MarkTree *ret = xcalloc(1,sizeof(*ret));
  ret->rel = rel;
  return ret;
}

#define mt_generic_cmp(a, b) (((b) < (a)) - ((a) < (b)))
static int key_cmp(mtkey_t a, mtkey_t b)
{
  int cmp = mt_generic_cmp(a.pos.row, b.pos.row);
  if (cmp != 0) {
    return cmp;
  }
  cmp = mt_generic_cmp(a.pos.col, b.pos.col);
  if (cmp != 0) {
    return cmp;
  }
  // NB: keeping the events at the same pos sorted by id is actually not
  // necessary only make sure that START is before END etc.
  return mt_generic_cmp(a.id, b.id);
}

static inline int marktree_getp_aux(const mtnode_t *x, mtkey_t k, int *r)
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

// put functions

// x must be an internal node, which is not full
// x->ptr[i] should be a full node, i e x->ptr[i]->n == 2*T-1
static inline void split_node(MarkTree *b, mtnode_t *x, const int i)
{
  mtnode_t *y = x->ptr[i];
  mtnode_t *z;
  z = (mtnode_t *)xcalloc(1, y->level ? ILEN : sizeof(mtnode_t));
  b->n_nodes++;
  z->level = y->level;
  z->n = T - 1;
  memcpy(z->key, &y->key[T], sizeof(mtkey_t) * (T - 1));
  for (int j = 0; j < T-1; j++) {
    refkey(b, z, j);
  }
  if (y->level) {
    memcpy(z->ptr, &y->ptr[T], sizeof(mtnode_t *) * T);
    for (int j = 0; j < T; j++) {
      z->ptr[j]->parent = z;
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

  if (b->rel) {
    for (int j = 0; j < T-1; j++) {
      relative(x->key[i].pos, &z->key[j].pos);
    }
    if (i > 0) {
      unrelative(x->key[i-1].pos, &x->key[i].pos);
    }
  }
}

// x must not be a full node (even if there might be internal space)
static inline void marktree_putp_aux(MarkTree *b, mtnode_t *x, mtkey_t k)
{
  int i = x->n - 1;
  if (x->level == 0) {
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
      split_node(b, x, i);
      if (key_cmp(k, x->key[i]) > 0) {
        i++;
      }
    }
    if (b->rel && i > 0) {
      relative(x->key[i-1].pos, &k.pos);
    }
    marktree_putp_aux(b, x->ptr[i], k);
  }
}

void marktree_put(MarkTree *b, mtkey_t k)
{
  if (!b->root) {
    b->root = (mtnode_t *)xcalloc(1, ILEN);
    b->id2node = pmap_new(uint64_t)();
    b->n_nodes++;
  }
  mtnode_t *r, *s;
  b->n_keys++;
  r = b->root;
  if (r->n == 2 * T - 1) {
    b->n_nodes++;
    s = (mtnode_t *)xcalloc(1, ILEN);
    b->root = s; s->level = r->level+1; s->n = 0;
    s->ptr[0] = r;
    r->parent = s;
    split_node(b, s, 0);
    r = s;
  }
  marktree_putp_aux(b, r, k);
}

void marktree_put_pos(MarkTree *b, int row, int col, uint64_t id)
{
  marktree_put(b, (mtkey_t){ .pos = (mtpos_t){ .row = row, .col = col },
                             .id = id });
}

/// INITIATING DELETION PROTOCOL:
///
/// 1. Construct a valid iterator to the node to delete (argument)
/// 2. If an "internal" key. Iterate one step to the left or right,
///     which gives an internal key "auxiliary key".
/// 3. Now delete this internal key (intended or auxiliary).
///    The leaf node X might become undersized.
/// 4. If step two was done: now replace the key that _should_ be
///    deleted with the auxiliary key. Adjust relative
/// 5. Now "repair" the tree as needed. We always start at a leaf node X.
///     - if the node is big enough, terminate
///     - if we can steal from the left, steal
///     - if we can steal from the right, steal
///     - otherwise merge this node with a neighbour. This might make our
///       parent undersized. So repeat 4 for the parent.
/// 6. If 4 went all the way to the root node. The root node
///    might have ended up with size 0. Delete it then.
///
/// NB: ideally keeps the iterator valid. Like point to the key after this
/// if present.
///
/// @param rev should be true if we plan to iterate _backwards_ and delete
///            stuff before this key. Most of the time this is false (the
///            recommended strategy is to always iterate forward)
void marktree_del_itr(MarkTree *b, MarkTreeIter *itr, bool rev)
{
  if (b->rel) {
    abort();
  }
  int adjustment = 0;
  
  mtnode_t *cur = itr->node;
  int curi = itr->i;

  if (itr->node->level) {
    if (rev) {
      abort();
    } else {
      //steal previous node
      marktree_itr_prev(b, itr);
      adjustment = -1;
    }
  }

  // 3.
  mtnode_t *x = itr->node;
  mtkey_t intkey = x->key[itr->i];
  if (x->n > itr->i+1) {
    memmove(&x->key[itr->i], &x->key[itr->i+1],
            sizeof(mtkey_t) * (size_t)(x->n - itr->i-1));
  }
  x->n--;

  // 4.
  if (adjustment) {
    if (adjustment == 1) {
      abort();
    } else { // adjustment == -1
      int ilvl = itr->lvl-1;
      mtnode_t *lnode = x;
      do {
        mtnode_t *p = lnode->parent;
        if (ilvl < 0) {
          abort();
        }
        int i = itr->s[ilvl].i;
        assert(p->ptr[i] == lnode);
        if (i > 0) {
          unrelative(lnode->key[i-1].pos, &intkey.pos);
        }
        lnode = p;
        ilvl--;
      } while (lnode != cur);
      abort();
    }
    // if adjustment == -1, need to unrelative from x up to cur
    //refkey();
  }

  // 5.
  int rlvl = itr->lvl;
  while (x != b->root) {
    mtnode_t *p = x->parent;
    if (x->n >= T-1) {
      // we are done, if this node is fine the rest of the tree will be
      break;
    }
    int pi  = itr->s[rlvl-1].i;
    assert(p->ptr[pi] == x);
    if (pi > 0 && p->ptr[pi-1]->n > T-1) {
      // steal one key from the left neighbour
      pivot_right(b, p, pi-1);
      break;
    } else if (pi < p->n && p->ptr[pi+1]->n > T-1) {
      // steal one key from right neighbour
      pivot_left(b, p, pi);
      break;
    } else if (pi > 0) {
      assert(p->ptr[pi-1]->n == T-1);
      // merge with left neighbour
      merge_node(b, p, pi-1);
    } else {
      assert(pi < p->n && p->ptr[pi+1]->n == T-1);
      merge_node(b, p, pi);
    }
  }

  // 6.
  if (b->root->n == 0) {
    abort();
    // TODO: always the case right?
    if (itr->lvl > 0) {
      memmove(itr->s, itr->s+1, (size_t)itr->lvl * sizeof(*itr->s));
      itr->lvl--;
    }
  }
}

// TODO: take in an iterator and "take care" of it? or caller should do that?
static void merge_node(MarkTree *b, mtnode_t *p, int i)
{
  mtnode_t *x = p->ptr[i], *y = p->ptr[i+1];

  x->key[x->n] = p->key[i];
  refkey(b, x, x->n);
  if (b->rel && i > 0) {
    relative(p->key[i-1].pos, &x->key[x->n].pos);
  }

  memmove(&x->key[x->n+1], y->key, (size_t)y->n * sizeof(mtkey_t));
  for (int k = 0; k < y->n; k++) {
    refkey(b, x, x->n+1+k);
    if (b->rel) {
      unrelative(x->key[x->n].pos, &x->key[x->n+1+k].pos);
    }
  }
  if (x->level) {
    memmove(&x->ptr[x->n], y->ptr, (size_t)(y->n + 1) * sizeof(mtkey_t *));
    for (int k = 0; k < y->n+1; k++) {
      x->ptr[x->n+k]->parent = x;
    }
  }
  x->n += y->n+1;
  memmove(&p->key[i], &p->key[i + 1], (size_t)(p->n - i - 1) * sizeof(mtkey_t));
  memmove(&p->ptr[i + 1], &p->ptr[i + 2],
          (size_t)(p->n - i - 1) * sizeof(mtkey_t *));
}

static void pivot_right(MarkTree *b, mtnode_t *p, int i)
{
  mtnode_t *x = p->ptr[i], *y = p->ptr[i+1];
  memmove(&y->key[1], y->key, (size_t)y->n * sizeof(mtkey_t));
  if (y->level) {
    memmove(&y->ptr[1], y->ptr, (size_t)(y->n + 1) * sizeof(mtkey_t *));
  }
  y->key[0] = p->key[i];
  refkey(b, y, 0);
  p->key[i] = x->key[x->n - 1];
  refkey(b, p, i);
  if (x->level) {
    y->ptr[0] = x->ptr[x->n];
    y->ptr[0]->parent = y;
  }
  x->n--;
  y->n++;
  if (b->rel) {
    if (i > 0) {
      unrelative(p->key[i-1].pos, &p->key[i].pos);
    }
    relative(p->key[i].pos, &y->key[0].pos);
    for (int k = 1; k < y->n; k++) {
      unrelative(y->key[0].pos, &y->key[k].pos);
    }
  }
}

static void pivot_left(MarkTree *b, mtnode_t *p, int i)
{
  mtnode_t *x = p->ptr[i], *y = p->ptr[i+1];
  if (b->rel) {
    // reverse from how we "always" do it. but pivot_left
    // is just the inverse of pivot_right, so reverse it literally.
    for (int k = 1; k < y->n; k++) {
      relative(y->key[0].pos, &y->key[k].pos);
    }
    unrelative(p->key[i].pos, &y->key[0].pos);
    if (i > 0) {
      relative(p->key[i-1].pos, &p->key[i].pos);
    }
  }
  x->key[x->n] = p->key[i];
  refkey(b, x, x->n);
  p->key[i] = y->key[0];
  refkey(b, p, i);
  if (x->level) {
    x->ptr[x->n+1] = y->ptr[0];
    x->ptr[x->n+1]->parent = x;
  }
  memmove(y->key, &y->key[1], (size_t)(y->n-1) * sizeof(mtkey_t));
  if (y->level) {
    memmove(y->ptr, &y->ptr[1], (size_t)y->n * sizeof(mtkey_t *));
  }
  x->n++;
  y->n--;
}

void marktree_check(MarkTree *b) {
  if (b->root == NULL) {
    assert(b->n_keys == 0);
    assert(b->n_nodes == 0);
    assert(map_size(b->id2node) == 0);
    return;
  }

  int nkeys = check_node(b, b->root);
  assert(b->n_keys == nkeys);
  assert(b->n_keys == (int)map_size(b->id2node));
}

int check_node(MarkTree *b, mtnode_t *x)
{
  assert(x->n <= 2*T-1);
  //TODO: too strick if checking "in repair" post-delete tree.
  assert(x->n >= T-1);

  

}

// itr functions

int marktree_failitr_get(MarkTree *b, mtkey_t k, MarkTreeIterFail *itr)
{
  if (b->n_keys == 0) {
    itr->p = NULL;
    return 0;
  }
  int i, r = 0;
  itr->p = itr->stack;
  itr->p->x = b->root;
  itr->pos.row = 0;
  itr->pos.col = 0;
  while (itr->p->x) {
    i = marktree_getp_aux(itr->p->x, k, &r);
    itr->p->i = i;
    if (i >= 0 && r == 0) {
      // TODO(bfredl): as a simplification we
      // could always be in the "before" state?
      return 1;
    }
    itr->p->i++;
    itr->p[1].x = itr->p->x->level ? itr->p->x->ptr[i + 1] : 0;
    if (b->rel && i >= 0) {
      unrelative(itr->p->x->key[i].pos, &itr->pos);
    }

    itr->p++;
  }
  return 0;
}

int marktree_failitr_next(MarkTree *b, MarkTreeIterFail *itr)
{
  if (itr->p < itr->stack) {
    return 0;
  }
  for (;;) {
    itr->p->i++;
    while (itr->p->x && itr->p->i <= itr->p->x->n) {
      itr->p[1].i = 0;
      if (itr->p->x->level) {
        itr->p[1].x = itr->p->x->ptr[itr->p->i];
        if (b->rel && itr->p->i > 0) {
          unrelative(itr->p->x->key[itr->p->i-1].pos, &itr->pos);
        }
      } else {
        itr->p[1].x = 0;
      }

      itr->p++;
    }

    itr->p--;
    if (itr->p < itr->stack) {
      return 0;
    }
    if (b->rel && itr->p->x->level && itr->p->i > 0) {
      relative(itr->p->x->key[itr->p->i-1].pos, &itr->pos);
    }
    if (itr->p->x && itr->p->i < itr->p->x->n) {
      return 1;
    }
  }
}

int marktree_failitr_prev(MarkTree *b, MarkTreeIterFail *itr)
{
  if (b->rel) {
    abort();  // häää
  }
  if (itr->p < itr->stack) {
    return 0;
  }
  for (;;) {
    while (itr->p->x && itr->p->i >= 0) {
      itr->p[1].x = itr->p->x->level ? itr->p->x->ptr[itr->p->i] : 0;
      itr->p[1].i = itr->p[1].x ? itr->p[1].x->n : -1;
      itr->p++;
    }
    itr->p--;
    if (itr->p < itr->stack) {
      return 0;
    }
    itr->p->i--;
    if (itr->p->x && itr->p->i >= 0) {
      return 1;
    }
  }
}

mtkey_t marktree_failitr_test(MarkTreeIterFail *itr)
{
  if ((itr)->p >= (itr)->stack) {
    mtkey_t key = ((itr)->p->x->key[(itr)->p->i]);
    unrelative(itr->pos, &key.pos);
    return key;
  }
  return (mtkey_t){ { -1, -1 }, 0 };
}

void marktree_itr_first(MarkTree *b, MarkTreeIter *itr)
{
  itr->node = b->root;
  if (!itr->node) {
    return;
  }

  itr->i = 0;
  itr->lvl = 0;
  while (itr->node->level > 0) {
    itr->s[itr->lvl].i = 0;
    itr->s[itr->lvl].oldcol = 0;
    itr->lvl++;
    itr->node = itr->node->ptr[0];
  }
}

bool marktree_itr_next(MarkTree *b, MarkTreeIter *itr)
{
  if (!itr->node) {
    return false;
  }
  itr->i++;
  if (itr->node->level == 0) {
    if (itr->i < itr->node->n) {
      // TODO: this is the common case, and should be handled by inline wrapper
      return true;
    }
    // we ran out of non-internal keys. Go up until we find a non-internal key
    while (itr->i >= itr->node->n) {
      itr->node = itr->node->parent;
      if (itr->node == NULL) {
        return false;
      }
      itr->lvl--;
      itr->i = itr->s[itr->lvl].i;
      if (b->rel && itr->i > 0) {
        itr->pos.row -= itr->node->key[itr->i-1].pos.row;
        itr->pos.col = itr->s[itr->lvl].oldcol;
      }
    }
  } else {
    // we stood at an "internal" key. Go down to the first non-internal
    // key after it.
    while (itr->node->level > 0) {
      // internal key, there is always a child after
      if (b->rel && itr->i > 0) {
        itr->s[itr->lvl].oldcol = itr->pos.col;
        mtpos_t ppos = itr->pos;
        itr->pos = itr->node->key[itr->i-1].pos;
        unrelative(ppos, &itr->pos);
      }
      itr->s[itr->lvl].i = itr->i;
      assert(itr->node->ptr[itr->i]->parent == itr->node);
      itr->node = itr->node->ptr[itr->i];
      itr->i = 0;
      itr->lvl++;
    }
  }
  return true;
}


mtkey_t marktree_itr_test(MarkTreeIter *itr)
{
  if (itr->node) {
    mtkey_t key = itr->node->key[itr->i];
    unrelative(itr->pos, &key.pos);
    return key;
  }
  return (mtkey_t){ { -1, -1 }, 0 };
}

void marktree_splice(MarkTree *b, mtpos_t start,
                     mtpos_t old_extent, mtpos_t new_extent)
{
  if (old_extent.row != 0 || old_extent.col != 0) {
    abort(); // NI! needs "delete"
  }
}

/// @param itr OPTIONAL. set itr to pos.
mtpos_t marktree_lookup(MarkTree *b, uint64_t id, MarkTreeIter *itr)
{
  if (!b->rel) {
    abort(); // no u!
  }
  mtnode_t *n = pmap_get(uint64_t)(b->id2node, id);
  if (n == NULL) {
    return (mtpos_t){ -1, -1 };
  }
  int i = 0;
  for (i = 0; i < n->n; i++) {
    if (n->key[i].id == id) {
      goto found;
    }
  }
  abort();
found: {}
  mtpos_t pos = n->key[i].pos;
  if (itr) {
    itr->i = i;
    itr->node = n;
    itr->lvl = b->root->level - n->level;
  }
  while (n->parent != NULL) {
    mtnode_t *p = n->parent;
    for (i = 0; i < p->n+1; i++) {
      if (p->ptr[i] == n) {
        goto found_node;
      }
    }
    abort();
found_node:
    if (itr) {
      itr->s[b->root->level-p->level].i = i;
    }
    if (i > 0) {
      unrelative(p->key[i-1].pos, &pos);
    }
    n = p;
  }
  if (itr) {
    itr->pos = (mtpos_t){ 0, 0 };
    mtnode_t *x = b->root;
    for (int lvl = 0; lvl < itr->lvl; lvl++) {
      itr->s[lvl].oldcol = itr->pos.col;
      i = itr->s[lvl].i;
      if (i > 0) {
        unrelative(x->key[i-1].pos, &itr->pos);
      }
      assert(x->level);
      x = x->ptr[i];
    }
    assert(x == itr->node);
  }
  return pos;
}

#if 0
String mt_inspect_iter(MarkTree *b)
{
  static char buf[1024];
  garray_T ga;
  ga_init(&ga, (int)sizeof(char), 80);
#define GA_PUT(x) ga_concat(&ga, (char_u *)(x))
  MarkTreeIterFail itr[1];
  mtkey_t k = { 0, 0, 0 };
  marktree_itr_get(b, k, itr);
  mtpos_t *lastp = itr->stack;
  for (; marktree_itr_next(b, itr); ) {
    while (itr->p < lastp) {
      GA_PUT(")");
      lastp--;
    }
    while (itr->p > lastp) {
      GA_PUT("(");
      lastp++;
    }
    mtkey_t inner = marktree_itr_test(itr);
    snprintf((char *)buf, sizeof(buf), "%d", inner.col);
    GA_PUT(buf);
    if (!itr->p->x->level) {
      GA_PUT(",");
    }
    lastp = itr->p;
  }
  while (itr->stack < lastp) {
    GA_PUT(")");
    lastp--;
  }
#undef GA_PUT
  return (String){ .data = ga.ga_data, .size = (size_t)ga.ga_len };
}
#endif

char *mt_inspect_rec(MarkTree *b)
{
  garray_T ga;
  ga_init(&ga, (int)sizeof(char), 80);
  mtpos_t p = { 0, 0 };
  mt_inspect_node(b, &ga, b->root, p);
  return ga.ga_data;
}

void mt_inspect_node(MarkTree *b, garray_T *ga, mtnode_t *n, mtpos_t off)
{
  static char buf[1024];
#define GA_PUT(x) ga_concat(ga, (char_u *)(x))
  GA_PUT("[");
  if (n->level) {
    mt_inspect_node(b, ga, n->ptr[0], off);
  }
  for (int i = 0; i < n->n; i++) {
    mtpos_t p = n->key[i].pos;
    if (b->rel) {
      unrelative(off, &p);
    }
    snprintf((char *)buf, sizeof(buf), "%d", p.col);
    GA_PUT(buf);
    if (n->level) {
      mt_inspect_node(b, ga, n->ptr[i+1], p);
    } else {
      GA_PUT(",");
    }
  }
  GA_PUT("]");
#undef GA_PUT
}

