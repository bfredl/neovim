// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Tree data structure for storing marks at (row, col) positions and updating
// them to arbitrary text changes. Derivative work of kbtree in klib, whose
// copyright notice is reproduced below. Also inspired by the design of the
// marker tree data structure of the Atom editor, regarding efficient updates
// to text changes.
//
// Marks are inserted using marktree_put. Text changes are processed using
// marktree_splice. All read and delete operations use the iterator.
// use marktree_itr_get to put an iterator at a given position or
// marktree_lookup to lookup a mark by its id (iterator optional in this case).
// Use marktree_itr_current and marktree_itr_next/prev to read marks in a loop.
// marktree_del_itr deletes the current mark of the iterator and implicitly
// moves the iterator to the next mark.
//
// Work is ongoing to fully support ranges (mark pairs).

// Copyright notice for kbtree (included in heavily modified form):
//
// Copyright 1997-1999, 2001, John-Mark Gurney.
//           2008-2009, Attractive Chaos <attractor@live.co.uk>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
// Changes done by by the neovim project follow the Apache v2 license available
// at the repo root.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "klib/kvec.h"
#include "nvim/garray.h"
#include "nvim/marktree.h"
#include "nvim/memory.h"
#include "nvim/pos.h"

// only for debug functions
#include "nvim/api/private/helpers.h"

#define T MT_BRANCH_FACTOR
#define ILEN (sizeof(mtnode_t) + (2 * T) * sizeof(void *))

#define ID_INCR (((uint64_t)1) << 2)

#define rawkey(itr) ((itr)->x->key[(itr)->i])

static bool pos_leq(mtpos_t a, mtpos_t b)
{
  return a.row < b.row || (a.row == b.row && a.col <= b.col);
}

static bool pos_less(mtpos_t a, mtpos_t b) { return !pos_leq(b, a); }

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

static void compose(mtpos_t *base, mtpos_t val)
{
  if (val.row == 0) {
    base->col += val.col;
  } else {
    base->row += val.row;
    base->col = val.col;
  }
}

typedef struct {
  uint64_t id;
  bool left; // if true, node was moved forward
  mtnode_t *old, *new;
  int old_i, new_i;
} Damage;
typedef kvec_withinit_t(Damage, 8) DamageList;


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "marktree.c.generated.h"
#endif

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

  // TODO(bfredl): MT_FLAG_REAL could go away if we fix marktree_getp_aux for real
  const uint16_t cmp_mask = MT_FLAG_RIGHT_GRAVITY | MT_FLAG_END | MT_FLAG_REAL | MT_FLAG_LAST;
  return mt_generic_cmp(a.flags & cmp_mask, b.flags & cmp_mask);
}

/// @return position of k if it exists in the node, otherwise the position
/// it should be inserted, which ranges from 0 to x->n _inclusively_
/// @param match (optional) set to TRUE if match (pos, gravity) was found
static inline int marktree_getp_aux(const mtnode_t *x, mtkey_t k, bool *match)
{
  bool dummy_match;
  bool *m =  match ? match : &dummy_match;

  int begin = 0, end = x->n;
  if (x->n == 0) {
    *m = false;
    return -1;
  }
  while (begin < end) {
    int mid = (begin + end) >> 1;
    if (key_cmp(x->key[mid], k) < 0) {
      begin = mid + 1;
    } else {
      end = mid;
    }
  }
  if (begin == x->n) {
    *m = false;
    return x->n-1;
  }
  if (!(*m = (key_cmp(k, x->key[begin]) == 0))) {
    begin--;
  }
  return begin;
}

static inline void refkey(MarkTree *b, mtnode_t *x, int i)
{
  pmap_put(uint64_t)(b->id2node, mt_lookup_key(x->key[i]), x);
}

static mtnode_t *id2node(MarkTree *b, uint64_t id)
{
  return pmap_get(uint64_t)(b->id2node, id);
}

// put functions

// x must be an internal node, which is not full
// x->ptr[i] should be a full node, i e x->ptr[i]->n == 2*T-1
static inline void split_node(MarkTree *b, mtnode_t *x, const int i, mtkey_t next)
{
  mtnode_t *y = x->ptr[i];
  mtnode_t *z = marktree_alloc_node(b, y->level);
  z->level = y->level;
  z->n = T - 1;

  // tricky: we might split a node in between inserting the start node and the end
  // node of the same pair. Then we must not intersect this id yet (done later
  // in marktree_intersect_pair).
  uint64_t last_start = mt_end(next) ? mt_lookup_id(next.ns, next.id, false) : MARKTREE_END_FLAG;

  // no alloc in the common case (less than 4 intersects)
  kvi_copy(z->intersect, y->intersect);

  if (!y->level) {
    for (int j = 0; j < T; j++) {
      mtkey_t k = y->key[j];
      if (mt_start(k) && id2node(b, mt_lookup_id(k.ns, k.id, true)) != y
          && mt_lookup_key(k) != last_start) {
        intersect_node(b, z, mt_lookup_id(k.ns, k.id, false));
      }
    }
    // note: y->key[T-1] is moved up and thus checked for both
    for (int j = T-1; j < (T * 2)-1; j++) {
      mtkey_t k = y->key[j];
      if (mt_end(k) && id2node(b, mt_lookup_id(k.ns, k.id, false)) != y) {
        intersect_node(b, y, mt_lookup_id(k.ns, k.id, false));
      }
    }
  }

  memcpy(z->key, &y->key[T], sizeof(mtkey_t) * (T - 1));
  for (int j = 0; j < T - 1; j++) {
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

  for (int j = 0; j < T - 1; j++) {
    relative(x->key[i].pos, &z->key[j].pos);
  }
  if (i > 0) {
    unrelative(x->key[i - 1].pos, &x->key[i].pos);
  }

  if (y->level) {
    bubble_up(y);
    bubble_up(z);
  } else {
    // code above goose here
  }
}

// x must not be a full node (even if there might be internal space)
static inline void marktree_putp_aux(MarkTree *b, mtnode_t *x, mtkey_t k)
{
  // TODO: ugh, make sure this is the _last_ valid (pos, gravity) position,
  // to minimize movement
  int i = marktree_getp_aux(x, k, NULL) + 1;
  if (x->level == 0) {
    if (i != x->n) {
      memmove(&x->key[i + 1], &x->key[i],
              (size_t)(x->n - i) * sizeof(mtkey_t));
    }
    x->key[i] = k;
    refkey(b, x, i);
    x->n++;
  } else {
    if (x->ptr[i]->n == 2 * T - 1) {
      split_node(b, x, i, k);
      if (key_cmp(k, x->key[i]) > 0) {
        i++;
      }
    }
    if (i > 0) {
      relative(x->key[i - 1].pos, &k.pos);
    }
    marktree_putp_aux(b, x->ptr[i], k);
  }
}

void marktree_put(MarkTree *b, mtkey_t key, int end_row, int end_col, bool end_right)
{
  assert(!(key.flags & ~MT_FLAG_EXTERNAL_MASK));
  if (end_row >= 0) {
    key.flags |= MT_FLAG_PAIRED;
  }

  marktree_put_key(b, key);

  if (end_row >= 0) {
    mtkey_t end_key = key;
    end_key.flags = (uint16_t)((uint16_t)(key.flags & ~MT_FLAG_RIGHT_GRAVITY)
                               |(uint16_t)MT_FLAG_END
                               |(uint16_t)(end_right ? MT_FLAG_RIGHT_GRAVITY : 0));
    end_key.pos = (mtpos_t){ end_row, end_col };
    marktree_put_key(b, end_key);
    MarkTreeIter itr[1] = { 0 }, end_itr[1] = { 0 };
    marktree_lookup(b, mt_lookup_key(key), itr);
    marktree_lookup(b, mt_lookup_key(end_key), end_itr);

    marktree_intersect_pair(b, mt_lookup_key(key), itr, end_itr, false);
  }
}

static void intersect_node(MarkTree *b, mtnode_t *x, uint64_t id)
{
  assert(!(id & MARKTREE_END_FLAG));
  kvi_pushp(x->intersect);
  // optimized for the common case: new key is always in the end
  for (ssize_t i = (ssize_t)kv_size(x->intersect)-1; i >= 0; i--) {
    if (i > 0 && kv_A(x->intersect, i-1) > id) {
      kv_A(x->intersect, i) = kv_A(x->intersect, i-1);
    } else {
      kv_A(x->intersect, i) = id;
      break;
    }
  }
}

static void unintersect_node(MarkTree *b, mtnode_t *x, uint64_t id, bool strict)
{
  assert(!(id & MARKTREE_END_FLAG));
  bool seen = false;
  for (size_t i = 0; i < kv_size(x->intersect); i++) {
    if (kv_A(x->intersect, i) < id) {
      continue;
    } else if (kv_A(x->intersect, i) == id) {
      seen = true;
    } else { // (kv_A(x->intersect, i) > id)
      if (strict) {
        assert(seen);
      } else {
        return;
      }
      // alternatively if(!seen) break;
    }

    if (i == kv_size(x->intersect)-1) {
      kv_size(x->intersect)--;
      break;
    }
    kv_A(x->intersect, i) = kv_A(x->intersect, i+1);
  }
  if (strict) {
    assert(seen);
  }
}

/// @param itr mutated
/// @param end_itr not mutated
void marktree_intersect_pair(MarkTree *b, uint64_t id, MarkTreeIter *itr, MarkTreeIter *end_itr, bool delete)
{
  int lvl = 0, maxlvl = MIN(itr->lvl, end_itr->lvl);
  for (; lvl < maxlvl; lvl++) {
    if (itr->s[lvl].i != end_itr->s[lvl].i) {
      break;
    }
  }

  while (itr->x) {
    bool skip = false;
    if (itr->x == end_itr->x) {
      if (itr->x->level == 0 || itr->i >= end_itr->i) {
        break;
      } else {
        skip = true;
      }
    } else if (itr->lvl > lvl) {
      skip = true;
    } else {
#define iat(itr, lvl) ((lvl == itr->lvl) ? itr->i+1 : itr->s[lvl].i)
      if (iat(itr, lvl) < iat(end_itr, lvl)) {
#undef iat
        skip = true;
      } else {
        lvl++;
      }
    }

    if (skip) {
      if (itr->x->level) {
        mtnode_t *x = itr->x->ptr[itr->i+1];
        if (delete) {
          unintersect_node(b, x, id, true);
        } else {
          intersect_node(b, x, id);
        }
      }
    }
    marktree_itr_next_skip(b, itr, skip, true, NULL);
  }
}

static mtnode_t *marktree_alloc_node(MarkTree *b, bool internal)
{
  mtnode_t *x = xcalloc(1, internal ? ILEN : sizeof(mtnode_t));
  kvi_init(x->intersect);
  b->n_nodes++;
  return x;
}

void marktree_put_key(MarkTree *b, mtkey_t k)
{
  k.flags |= MT_FLAG_REAL;  // let's be real.
  if (!b->root) {
    b->root = marktree_alloc_node(b, true);
  }
  mtnode_t *r, *s;
  b->n_keys++;
  r = b->root;
  if (r->n == 2 * T - 1) {
    s = marktree_alloc_node(b, true);
    b->root = s; s->level = r->level + 1; s->n = 0;
    s->ptr[0] = r;
    r->parent = s;
    split_node(b, s, 0, k);
    r = s;
  }
  marktree_putp_aux(b, r, k);
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
///       parent undersized. So repeat 5 for the parent.
/// 6. If 4 went all the way to the root node. The root node
///    might have ended up with size 0. Delete it then.
///
/// The iterator remains valid, and now points at the key _after_ the deleted
/// one.
///
/// @param rev should be true if we plan to iterate _backwards_ and delete
///            stuff before this key. Most of the time this is false (the
///            recommended strategy is to always iterate forward)
uint64_t marktree_del_itr(MarkTree *b, MarkTreeIter *itr, bool rev)
{
  int adjustment = 0;

  mtnode_t *cur = itr->x;
  int curi = itr->i;
  uint64_t id = mt_lookup_key(cur->key[curi]);
  // fprintf(stderr, "\nDELET %lu\n", id);

  mtkey_t raw = rawkey(itr);
  uint64_t other = 0;
  if (mt_paired(raw) && !(raw.flags & MT_FLAG_ORPHANED)) {
    other = mt_lookup_key_side(raw, !mt_end(raw));

    MarkTreeIter other_itr[1];
    marktree_lookup(b, other, other_itr);
    rawkey(other_itr).flags |= MT_FLAG_ORPHANED;
    // Remove intersect markers. NB: must match exactly!
    if (mt_start(raw)) {
      MarkTreeIter this_itr[1] = { *itr }; // mutated copy
      marktree_intersect_pair(b, id, this_itr, other_itr, true);
    } else {
      marktree_intersect_pair(b, other, other_itr, itr, true);
    }
  }

  if (itr->x->level) {
    if (rev) {
      abort();
    } else {
      // fprintf(stderr, "INTERNAL %d\n", cur->level);
      // steal previous node
      marktree_itr_prev(b, itr);
      adjustment = -1;
    }
  }

  // 3.
  mtnode_t *x = itr->x;
  assert(x->level == 0);
  mtkey_t intkey = x->key[itr->i];
  if (x->n > itr->i + 1) {
    memmove(&x->key[itr->i], &x->key[itr->i + 1],
            sizeof(mtkey_t) * (size_t)(x->n - itr->i - 1));
  }
  x->n--;

  // 4.
  // if (adjustment == 1) {
  //   abort();
  // }
  if (adjustment == -1) {
    int ilvl = itr->lvl - 1;
    const mtnode_t *lnode = x;
    do {
      const mtnode_t *const p = lnode->parent;
      if (ilvl < 0) {
        abort();
      }
      const int i = itr->s[ilvl].i;
      assert(p->ptr[i] == lnode);
      if (i > 0) {
        unrelative(p->key[i - 1].pos, &intkey.pos);
      }
      lnode = p;
      ilvl--;
    } while (lnode != cur);

    mtkey_t deleted = cur->key[curi];
    cur->key[curi] = intkey;
    refkey(b, cur, curi);
    if (mt_end(cur->key[curi])) {
      if (cur->level > 1) {
        // TODO: intkey was moved UP here. need to bubble intersections
        fprintf(stderr, "eeeeek\n");
        abort();
      } else {
        // TODO: this is very similar to code in pivot_right, abstract?
        uint64_t start_id = mt_lookup_key_side(cur->key[curi], false);
        mtkey_t start =  marktree_lookup(b, start_id, NULL);
        mtkey_t first = x->key[0];
        // make pos of first absolute: first pos relative cur instead of x, and then use p_pos
        if (curi > 0) {
          unrelative(cur->key[curi-1].pos, &first.pos);
        }
        // itr has pos of x
        unrelative(itr->pos, &first.pos);
        if (key_cmp(start, first) < 0) {
          // printf("intersect end\n"); fflush(enheten); // TODO: 
          intersect_node(b, x, start_id);
        }
      }
    }

    relative(intkey.pos, &deleted.pos);
    mtnode_t *y = cur->ptr[curi + 1];
    if (deleted.pos.row || deleted.pos.col) {
      while (y) {
        for (int k = 0; k < y->n; k++) {
          unrelative(deleted.pos, &y->key[k].pos);
        }
        y = y->level ? y->ptr[0] : NULL;
      }
    }
    itr->i--;
  }

  b->n_keys--;
  pmap_del(uint64_t)(b->id2node, id, NULL);

  // 5.
  bool itr_dirty = false;
  int rlvl = itr->lvl - 1;
  int *lasti = &itr->i;
  mtpos_t ppos = itr->pos;
  while (x != b->root) {
    assert(rlvl >= 0);
    mtnode_t *p = x->parent;
    if (x->n >= T - 1) {
      // we are done, if this node is fine the rest of the tree will be
      break;
    }
    int pi = itr->s[rlvl].i;
    assert(p->ptr[pi] == x);
    if (pi > 0) {
      ppos.row -= p->key[pi-1].pos.row;
      ppos.col = itr->s[rlvl].oldcol;
    }
    // ppos is now the pos of p

    if (pi > 0 && p->ptr[pi - 1]->n > T - 1) {
      // printf("pivot_right\n");fflush(stdout);
      *lasti += 1;
      itr_dirty = true;
      // steal one key from the left neighbour
      pivot_right(b, ppos, p, pi - 1);
      break;
    } else if (pi < p->n && p->ptr[pi + 1]->n > T - 1) {
      // printf("pivot_left\n");fflush(stdout);
      // steal one key from right neighbour
      pivot_left(b, ppos, p, pi);
      break;
    } else if (pi > 0) {
      // printf("merga\n");fflush(stdout);
      // fprintf(stderr, "LEFT ");
      assert(p->ptr[pi - 1]->n == T - 1);
      // merge with left neighbour
      *lasti += T;
      x = merge_node(b, p, pi - 1);
      if (lasti == &itr->i) {
        // TRICKY: we merged the node the iterator was on
        itr->x = x;
      }
      itr->s[rlvl].i--;
      itr_dirty = true;
    } else {
      // fprintf(stderr, "RIGHT ");
      assert(pi < p->n && p->ptr[pi + 1]->n == T - 1);
      merge_node(b, p, pi);
      // no iter adjustment needed
    }
    lasti = &itr->s[rlvl].i;
    rlvl--;
    x = p;
  }

  // 6.
  if (b->root->n == 0) {
    if (itr->lvl > 0) {
      memmove(itr->s, itr->s + 1, (size_t)(itr->lvl - 1) * sizeof(*itr->s));
      itr->lvl--;
    }
    if (b->root->level) {
      mtnode_t *oldroot = b->root;
      b->root = b->root->ptr[0];
      b->root->parent = NULL;
      marktree_free_node(b, oldroot);
    } else {
      // no items, nothing for iterator to point to
      // not strictly needed, should handle delete right-most mark anyway
      itr->x = NULL;
    }
  }

  if (itr->x && itr_dirty) {
    marktree_itr_fix_pos(b, itr);
  }

  // BONUS STEP: fix the iterator, so that it points to the key afterwards
  // TODO(bfredl): with "rev" should point before
  // if (adjustment == 1) {
  //   abort();
  // }
  if (adjustment == -1) {
    // tricky: we stand at the deleted space in the previous leaf node.
    // But the inner key is now the previous key we stole, so we need
    // to skip that one as well.
    marktree_itr_next(b, itr);
    marktree_itr_next(b, itr);
  } else {
    if (itr->x && itr->i >= itr->x->n) {
      // we deleted the last key of a leaf node
      // go to the inner key after that.
      assert(itr->x->level == 0);
      marktree_itr_next(b, itr);
    }
  }

  return other;
}

static void merge_intersection(Intersection *restrict m, Intersection *restrict x, Intersection *restrict y) {
  size_t xi = 0, yi = 0;
  size_t xn = 0, yn = 0;
  while (xi < kv_size(*x) && yi < kv_size(*y)) {
    if (kv_A(*x, xi) == kv_A(*y, yi)) {
        // TODO: kvi_pushp is actually quite complex, break out kvi_resize() to a function?
        kvi_push(*m, kv_A(*x, xi));
        xi++;
        yi++;
    } else if (kv_A(*x, xi) < kv_A(*y, yi)) {
      kv_A(*x, xn++) = kv_A(*x, xi++);
    } else {
      kv_A(*y, yn++) = kv_A(*y, yi++);
    }
  }

  // TODO: use memmove, special case xi=xn
  while (xi < kv_size(*x)) {
    kv_A(*x, xn++) = kv_A(*x, xi++);
  }
  while (yi < kv_size(*y)) {
    kv_A(*y, yn++) = kv_A(*y, yi++);
  }

  kv_size(*x) = xn;
  kv_size(*y) = yn;
}


// w used to be a child of x but it is now a child of y, adjust intersections accordingly
// @param[out] d are intersections which should be added to the old children of y
static void intersect_mov(Intersection *restrict x, Intersection *restrict y,
                          Intersection *restrict w, Intersection *restrict d)
{
  size_t wi = 0, yi = 0;
  size_t wn = 0, yn = 0;
  size_t xi = 0;
  while (wi < kv_size(*w) || xi < kv_size(*x)) {
    if (wi < kv_size(*w) && (xi >= kv_size(*x) || kv_A(*x, xi) >= kv_A(*w, wi))) {
      if (xi < kv_size(*x) && kv_A(*x, xi) == kv_A(*w, wi)) {
        xi++;
      }
      // now w < x strictly
      while (yi < kv_size(*y) && kv_A(*y, yi) < kv_A(*w, wi)) {
        kvi_push(*d, kv_A(*y, yi));
        yi++;
      }
      if (yi < kv_size(*y) && kv_A(*y, yi) == kv_A(*w, wi)) {
        kv_A(*y, yn++) = kv_A(*y, yi++);
        wi++;
      } else {
        kv_A(*w, wn++) = kv_A(*w, wi++);
      }
    } else {
      // x < w strictly
      while (yi < kv_size(*y) && kv_A(*y, yi) < kv_A(*x, xi)) {
        kvi_push(*d, kv_A(*y, yi));
        yi++;
      }
      if (yi < kv_size(*y) && kv_A(*y, yi) == kv_A(*x, xi)) {
        kv_A(*y, yn++) = kv_A(*y, yi++);
        xi++;
      } else {
        // add kv_A(x, xi) at kv_A(w, wn), pushing up wi if wi == wn
        if (wi == wn) {
          size_t n = kv_size(*w) - wn;
          kvi_pushp(*w);
          if (n > 0) {
            memmove(&kv_A(*w, wn+1), &kv_A(*w, wn), n*sizeof(kv_A(*w, 0)));
          }
          kv_A(*w, wi) = kv_A(*x, xi);
          wn++;
          wi++; // no need to consider the added element again
        } else {
          assert(wn < wi);
          kv_A(*w, wn++) = kv_A(*x, xi);
        }
        xi++;
      }
    }
  }
  if (yi < kv_size(*y)) {
    // move remaining items to d
    size_t n = kv_size(*y) - yi;  // at least one
    kvi_ensure_more_space(*d, n);
    memcpy(&kv_A(*d, kv_size(*d)), &kv_A(*y, yi), n*sizeof(kv_A(*d, 0)));
    kv_size(*d) += n;
  }
  kv_size(*w) = wn;
  kv_size(*y) = yn;
}

bool intersect_mov_test(uint64_t *x, size_t nx,
                        uint64_t *y, size_t ny,
                        uint64_t *win, size_t nwin,
                        uint64_t *wout, size_t *nwout,
                        uint64_t *dout, size_t *ndout
                        ) {
  // these are immutable in the context of intersect_mov
  Intersection xi = {.items = x, .size = nx };
  Intersection yi = {.items = y, .size = ny };

  Intersection w;
  kvi_init(w);
  for (size_t i = 0; i < nwin; i++) {
    kvi_push(w, win[i]);
  }
  Intersection d;
  kvi_init(d);

  intersect_mov(&xi, &yi, &w, &d);

  if (w.size > *nwout || d.size > *ndout) {
    return false;
  }

  memcpy(wout, w.items, sizeof(w.items[0])*w.size);
  *nwout = w.size;
  //printf("\n\nweee %lu %lu %lu\n", w.size, w.items[0], w.items[1]);
  // printf("nwoooeee %lu %lu %lu\n", *nwout, wout[0], wout[1]);
  // fflush(stdout);

  memcpy(dout, d.items, sizeof(d.items[0])*d.size);
  *ndout = d.size;

  return true;
}

static void intersect(Intersection *i, Intersection *x, Intersection *y)
{
  size_t xi = 0, yi = 0;
  while (xi < kv_size(*x) && yi < kv_size(*y)) {
    if (kv_A(*x, xi) == kv_A(*y, yi)) {
        kvi_push(*i, kv_A(*x, xi));
        xi++;
        yi++;
    } else if (kv_A(*x, xi) < kv_A(*y, yi)) {
      xi++;
    } else {
      yi++;
    }
  }
}

// inplace union: x |= y
static void intersect_add(Intersection *x, Intersection *y)
{
  size_t xi = 0, yi = 0;
  while (xi < kv_size(*x) && yi < kv_size(*y)) {
    if (kv_A(*x, xi) == kv_A(*y, yi)) {
      xi++;
      yi++;
    } else if (kv_A(*y, yi) < kv_A(*x, xi)) {
      size_t n = kv_size(*x) - xi;  // at least one
      // printf("moved %lu because %d vs %d\n", n, );
      kvi_pushp(*x);
      memmove(&kv_A(*x, xi+1), &kv_A(*x, xi), n*sizeof(kv_A(*x, 0)));
      kv_A(*x, xi) = kv_A(*y, yi);
      xi++; // newly added element
      yi++;
    } else {
      xi++;
    }
  }
  if (yi < kv_size(*y)) {
      size_t n = kv_size(*y) - yi;  // at least one
      kvi_ensure_more_space(*x, n);
      memcpy(&kv_A(*x, kv_size(*x)), &kv_A(*y, yi), n*sizeof(kv_A(*x, 0)));
      kv_size(*x) += n;
  }
}

// inplace assymetric difference: x &= ~y
static void intersect_sub(Intersection *restrict x, Intersection *restrict y) {
  size_t xi = 0, yi = 0;
  size_t xn = 0;
  while (xi < kv_size(*x) && yi < kv_size(*y)) {
    if (kv_A(*x, xi) == kv_A(*y, yi)) {
      xi++;
      yi++;
    } else if (kv_A(*x, xi) < kv_A(*y, yi)) {
      kv_A(*x, xn++) = kv_A(*x, xi++);
    } else {
      yi++;
    }
  }
  if (xi < kv_size(*x)) {
    size_t n = kv_size(*x) - xi;
    if (xn < xi) {  // otherwise xn == xi
      memmove(&kv_A(*x, xn), &kv_A(*x, xi), n*sizeof(kv_A(*x, 0)));
    }
    xn += n;
  }
  kv_size(*x) = xn;
}

static void bubble_up(mtnode_t *x)
{
  Intersection xi;
  kvi_init(xi);
  intersect(&xi, &x->ptr[0]->intersect, &x->ptr[x->n]->intersect);
  if (kv_size(xi)) {
    for (int i = 0; i < x->n+1; i++) {
      intersect_sub(&x->ptr[i]->intersect, &xi);
    }
    intersect_add(&x->intersect, &xi);
  }
  kvi_destroy(xi);
}

static mtnode_t *merge_node(MarkTree *b, mtnode_t *p, int i)
{
  mtnode_t *x = p->ptr[i], *y = p->ptr[i + 1];
  Intersection m;
  kvi_init(m);

  merge_intersection(&m, &x->intersect, &y->intersect);

  x->key[x->n] = p->key[i];
  refkey(b, x, x->n);
  if (i > 0) {
    relative(p->key[i - 1].pos, &x->key[x->n].pos);
  }

  memmove(&x->key[x->n + 1], y->key, (size_t)y->n * sizeof(mtkey_t));
  for (int k = 0; k < y->n; k++) {
    refkey(b, x, x->n + 1 + k);
    unrelative(x->key[x->n].pos, &x->key[x->n + 1 + k].pos);
  }
  if (x->level) {
    // bubble down: ranges that intersected old-x but not old-y or vice versa
    // must be moved to their respective children
    memmove(&x->ptr[x->n + 1], y->ptr, ((size_t)y->n + 1) * sizeof(mtnode_t *));
    for (int k = 0; k < x->n + 1; k++) {
      // TODO(bfredl): dedicated impl for "Z |= Y"
      for (size_t idx = 0; idx < kv_size(x->intersect); idx++) {
        intersect_node(b,x->ptr[k],kv_A(x->intersect, idx));
      }
    }
    for (int ky = 0; ky < y->n + 1; ky++) {
      int k = x->n + ky + 1;
      // nodes that used to be in y, now the second half of x
      x->ptr[k]->parent = x;
      // TODO(bfredl): dedicated impl for "Z |= X"
      for (size_t idx = 0; idx < kv_size(y->intersect); idx++) {
        intersect_node(b,x->ptr[k],kv_A(y->intersect, idx));
      }
    }
  }
  x->n += y->n + 1;
  memmove(&p->key[i], &p->key[i + 1], (size_t)(p->n - i - 1) * sizeof(mtkey_t));
  memmove(&p->ptr[i + 1], &p->ptr[i + 2],
          (size_t)(p->n - i - 1) * sizeof(mtkey_t *));
  p->n--;
  marktree_free_node(b, y);

  kvi_destroy(x->intersect);

  // move of a kvec_withinit_t, messy!
  // special case a version of merge_intersection(x_out, x_in_m_out, y) to avoid
  // thisi
  kvi_move(&x->intersect, &m);

  return x;
}

/// @param dest is overwritten (assumed to already been freed/moved)
/// @param src consumed (don't free or use)
void kvi_move(Intersection *dest, Intersection* src)
{
  dest->size = src->size;
  dest->capacity = src->capacity;
  if (src->items == src->init_array) {
    memcpy(dest->init_array, src->init_array, src->size*sizeof(*src->init_array));
    dest->items = dest->init_array;
  } else {
    dest->items = src->items;
  }
}


// TODO(bfredl): as a potential "micro" optimization, pivoting should balance
// the two nodes instead of stealing just one key
// x_pos is the absolute position of the key just before x (or a dummy key strictly less than any
// key inside x, if x is the first leaf)
static void pivot_right(MarkTree *b, mtpos_t p_pos, mtnode_t *p, const int i)
{
  mtnode_t *x = p->ptr[i], *y = p->ptr[i + 1];
  memmove(&y->key[1], y->key, (size_t)y->n * sizeof(mtkey_t));
  if (y->level) {
    memmove(&y->ptr[1], y->ptr, ((size_t)y->n + 1) * sizeof(mtnode_t *));
  }
  // printf("old %d, ", p->key[i].id);
  y->key[0] = p->key[i];
  refkey(b, y, 0);
  p->key[i] = x->key[x->n - 1];
  // printf("new %d\n", p->key[i].id);
  refkey(b, p, i);
  if (x->level) {
    y->ptr[0] = x->ptr[x->n];
    y->ptr[0]->parent = y;
  }
  x->n--;
  y->n++;
  if (i > 0) {
    unrelative(p->key[i - 1].pos, &p->key[i].pos);
  }
  relative(p->key[i].pos, &y->key[0].pos);
  for (int k = 1; k < y->n; k++) {
    unrelative(y->key[0].pos, &y->key[k].pos);
  }

  // repair intersections of x
  if (x->level) {
    // handle y and first new y->ptr[0]
    Intersection d;
    kvi_init(d);
    // y->ptr[0] was moved from x to y
    // adjust y->ptr[0] for a difference between the parents
    // in addition, this might cause some intersection of the old y
    // to bubble down to the old children of y (if y->ptr[0] wasn't intersected)
    intersect_mov(&x->intersect, &y->intersect, &y->ptr[0]->intersect, &d);
    if (kv_size(d)) {
      // printf("Coverage: X-right");  // TODO:
      for (int yi = 1; yi < y->n+1; yi++) {
        intersect_add(&y->ptr[yi]->intersect, &d);
      }
    }
    kvi_destroy(d);

    bubble_up(x);
  } else {
    // if the last element of x used to be an end node, check if it now covers all of x
    if (mt_end(p->key[i])) {
      uint64_t start_id = mt_lookup_key_side(p->key[i], false);
      mtkey_t start =  marktree_lookup(b, start_id, NULL);
      mtkey_t first = x->key[0];
      // make pos of first absolute: first pos relative p instead of x, and then use p_pos
      if (i > 0) {
        unrelative(p->key[i-1].pos, &first.pos);
      }
      unrelative(p_pos, &first.pos);
      if (key_cmp(start, first) < 0) {
        // printf("intersect end\n"); fflush(stdout); // TODO: 
        intersect_node(b, x, start_id);
      }
    }

    if (mt_start(y->key[0])) {
      // no need for a check, just delet it if it was there
      unintersect_node(b, y, mt_lookup_key(y->key[0]), false);
    }
  }

}

static void pivot_left(MarkTree *b, mtpos_t p_pos, mtnode_t *p, int i)
{
  mtnode_t *x = p->ptr[i], *y = p->ptr[i + 1];

  // reverse from how we "always" do it. but pivot_left
  // is just the inverse of pivot_right, so reverse it literally.
  for (int k = 1; k < y->n; k++) {
    relative(y->key[0].pos, &y->key[k].pos);
  }
  unrelative(p->key[i].pos, &y->key[0].pos);
  if (i > 0) {
    relative(p->key[i - 1].pos, &p->key[i].pos);
  }

  x->key[x->n] = p->key[i];
  refkey(b, x, x->n);
  p->key[i] = y->key[0];
  refkey(b, p, i);
  if (x->level) {
    x->ptr[x->n + 1] = y->ptr[0];
    x->ptr[x->n + 1]->parent = x;
  }
  memmove(y->key, &y->key[1], (size_t)(y->n - 1) * sizeof(mtkey_t));
  if (y->level) {
    memmove(y->ptr, &y->ptr[1], (size_t)y->n * sizeof(mtnode_t *));
  }
  x->n++;
  y->n--;


  // repair intersections of x,y
  if (x->level) {
    // handle y and first new y->ptr[0]
    Intersection d;
    kvi_init(d);
    // x->ptr[x->n] was moved from y to x
    // adjust x->ptr[x->n] for a difference between the parents
    // in addition, this might cause some intersection of the old y
    // to bubble down to the old children of y (if y->ptr[0] wasn't intersected)
    intersect_mov(&y->intersect, &x->intersect, &x->ptr[x->n]->intersect, &d);
    if (kv_size(d)) {
      // printf("Coverage: X-left");  // TODO:
      for (int xi = 0; xi < x->n; xi++) { // ptr[x->n| deliberately skipped
        intersect_add(&y->ptr[xi]->intersect, &d);
      }
    }
    kvi_destroy(d);

    bubble_up(y);
  } else {
    // if the first element of y used to be an start node, check if it now covers all of y
    if (mt_start(p->key[i])) {
      uint64_t end_id = mt_lookup_key_side(p->key[i], true);
      mtkey_t end =  marktree_lookup(b, end_id, NULL);
      mtkey_t last = y->key[y->n-1];

      // make pos of last absolute: first pos relative p instead of y, and then use p_pos
      unrelative(p->key[i].pos, &last.pos);
      unrelative(p_pos, &last.pos);
      if (key_cmp(end, last) > 0) {
        // printf("intersect start\n"); fflush(enheten); // TODO:
        intersect_node(b, y, mt_lookup_key(p->key[i]));
      }
    }

    if (mt_end(x->key[x->n-1])) {
      // no need for a check, just delet it if it was there
      unintersect_node(b, x, mt_lookup_key_side(x->key[x->n-1], false), false);
    }
  }
}

/// frees all mem, resets tree to valid empty state
void marktree_clear(MarkTree *b)
{
  if (b->root) {
    marktree_free_subtree(b, b->root);
    b->root = NULL;
  }
  if (b->id2node->table.keys) {
    map_destroy(uint64_t, b->id2node);
    *b->id2node = (PMap(uint64_t)) MAP_INIT;
  }
  b->n_keys = 0;
  assert(b->n_nodes == 0);
}

void marktree_free_subtree(MarkTree *b, mtnode_t *x)
{
  if (x->level) {
    for (int i = 0; i < x->n+1; i++) {
      marktree_free_subtree(b, x->ptr[i]);
    }
  }
  marktree_free_node(b, x);
}

static void marktree_free_node(MarkTree *b, mtnode_t *x)
{
  kvi_destroy(x->intersect);
  xfree(x);
  b->n_nodes--;
}

/// NB: caller must check not pair!
void marktree_revise(MarkTree *b, MarkTreeIter *itr, uint8_t decor_level, mtkey_t key)
{
  // TODO(bfredl): clean up this mess and re-instantiate &= and |= forms
  // once we upgrade to a non-broken version of gcc in functionaltest-lua CI
  rawkey(itr).flags = (uint16_t)(rawkey(itr).flags & (uint16_t) ~MT_FLAG_DECOR_MASK);
  rawkey(itr).flags = (uint16_t)(rawkey(itr).flags
                                 | (uint16_t)(decor_level << MT_FLAG_DECOR_OFFSET)
                                 | (uint16_t)(key.flags & MT_FLAG_DECOR_MASK));
  rawkey(itr).decor_full = key.decor_full;
  rawkey(itr).hl_id = key.hl_id;
  rawkey(itr).priority = key.priority;
}

/// @param itr iterator is invalid after call
void marktree_move(MarkTree *b, MarkTreeIter *itr, int row, int col)
{
  mtkey_t key = rawkey(itr);
  mtnode_t *x = itr->x;
  if (!x->level) {
    bool internal = false;
    mtpos_t newpos = mtpos_t(row, col);
    if (x->parent != NULL) {
      // strictly _after_ key before `x`
      // (not optimal when x is very first leaf of the entire tree, but that's fine)
      if (pos_less(itr->pos, newpos)) {
        relative(itr->pos, &newpos);

        // strictly before the end of x. (this could be made sharper by
        // finding the internal key just after x, but meh)
        if (pos_less(newpos, x->key[x->n-1].pos)) {
          internal = true;
        }
      }
    } else {
      // tree is one node. newpos thus is already "relative" itr->pos
      internal = true;
    }

    if (internal) {
      key.pos = newpos;
      bool match;
      // tricky: could minimize movement in either direction better
      int new_i = marktree_getp_aux(x, key, &match);
      if (!match) {
        new_i++;
      }
      if (new_i == itr->i || key_cmp(key, x->key[new_i]) == 0) {
        x->key[itr->i].pos = newpos;
      } else if (new_i < itr->i) {
        memmove(&x->key[new_i+1], &x->key[new_i], sizeof(mtkey_t)*(size_t)(itr->i - new_i));
        x->key[new_i] = key;
      } else if (new_i > itr->i) {
        memmove(&x->key[itr->i], &x->key[itr->i+1], sizeof(mtkey_t)*(size_t)(new_i - itr->i));
        x->key[new_i] = key;
      }
      return;
    }

  }
  uint64_t other = marktree_del_itr(b, itr, false);
  key.pos = (mtpos_t){ row, col };

  marktree_put_key(b, key);

  if (other) {
    marktree_restore_pair(b, key);
  }
  itr->x = NULL;  // itr might become invalid by put
}

void marktree_restore_pair(MarkTree *b, mtkey_t key) {
    MarkTreeIter itr[1];
    MarkTreeIter end_itr[1];
    marktree_lookup(b, mt_lookup_key_side(key, false), itr);
    marktree_lookup(b, mt_lookup_key_side(key, true), end_itr);
    if (!itr->x || !end_itr->x) {
      // this could happen if the other end is waiting to be restored later
      // this function will be called again for the other end.
      return;
    }
    rawkey(itr).flags &= (uint16_t)~MT_FLAG_ORPHANED;
    rawkey(end_itr).flags &= (uint16_t)~MT_FLAG_ORPHANED;

    marktree_intersect_pair(b, mt_lookup_key_side(key, false), itr, end_itr, false);
}

// itr functions

// TODO(bfredl): static inline?
bool marktree_itr_get(MarkTree *b, int32_t row, int col, MarkTreeIter *itr)
{
  return marktree_itr_get_ext(b, mtpos_t(row, col), itr, false, false, NULL);
}

bool marktree_itr_get_ext(MarkTree *b, mtpos_t p, MarkTreeIter *itr, bool last, bool gravity,
                          mtpos_t *oldbase)
{
  if (b->n_keys == 0) {
    itr->x = NULL;
    return false;
  }

  mtkey_t k = { .pos = p, .flags = gravity ? MT_FLAG_RIGHT_GRAVITY : 0 };
  if (last && !gravity) {
    k.flags = MT_FLAG_LAST;
  }
  itr->pos = (mtpos_t){ 0, 0 };
  itr->x = b->root;
  itr->lvl = 0;
  if (oldbase) {
    oldbase[itr->lvl] = itr->pos;
  }
  while (true) {
    itr->i = marktree_getp_aux(itr->x, k, 0) + 1;

    if (itr->x->level == 0) {
      break;
    }

    itr->s[itr->lvl].i = itr->i;
    itr->s[itr->lvl].oldcol = itr->pos.col;

    if (itr->i > 0) {
      compose(&itr->pos, itr->x->key[itr->i - 1].pos);
      relative(itr->x->key[itr->i - 1].pos, &k.pos);
    }
    itr->x = itr->x->ptr[itr->i];
    itr->lvl++;
    if (oldbase) {
      oldbase[itr->lvl] = itr->pos;
    }
  }

  if (last) {
    return marktree_itr_prev(b, itr);
  } else if (itr->i >= itr->x->n) {
    return marktree_itr_next(b, itr);
  }
  return true;
}

bool marktree_itr_first(MarkTree *b, MarkTreeIter *itr)
{
  itr->x = b->root;
  if (b->n_keys == 0) {
    return false;
  }

  itr->i = 0;
  itr->lvl = 0;
  itr->pos = mtpos_t(0, 0);
  while (itr->x->level > 0) {
    itr->s[itr->lvl].i = 0;
    itr->s[itr->lvl].oldcol = 0;
    itr->lvl++;
    itr->x = itr->x->ptr[0];
  }
  return true;
}

// gives the first key that is greater or equal to p
int marktree_itr_last(MarkTree *b, MarkTreeIter *itr)
{
  if (b->n_keys == 0) {
    itr->x = NULL;
    return false;
  }
  itr->pos = mtpos_t(0, 0);
  itr->x = b->root;
  itr->lvl = 0;
  while (true) {
    itr->i = itr->x->n;

    if (itr->x->level == 0) {
      break;
    }

    itr->s[itr->lvl].i = itr->i;
    itr->s[itr->lvl].oldcol = itr->pos.col;

    assert(itr->i > 0);
    compose(&itr->pos, itr->x->key[itr->i - 1].pos);

    itr->x = itr->x->ptr[itr->i];
    itr->lvl++;
  }
  itr->i--;
  return true;
}


// TODO(bfredl): static inline
bool marktree_itr_next(MarkTree *b, MarkTreeIter *itr)
{
  return marktree_itr_next_skip(b, itr, false, false, NULL);
}

static bool marktree_itr_next_skip(MarkTree *b, MarkTreeIter *itr, bool skip,
                                   bool preload, mtpos_t oldbase[])
{
  if (!itr->x) {
    return false;
  }
  itr->i++;
  if (itr->x->level == 0 || skip) {
    if (preload && itr->x->level == 0 && skip) {
      // skip rest of this leaf node
      itr->i = itr->x->n;
    } else if (itr->i < itr->x->n) {
      // TODO(bfredl): this is the common case,
      // and could be handled by inline wrapper
      return true;
    }
    // we ran out of non-internal keys. Go up until we find an internal key
    while (itr->i >= itr->x->n) {
      itr->x = itr->x->parent;
      if (itr->x == NULL) {
        return false;
      }
      itr->lvl--;
      itr->i = itr->s[itr->lvl].i;
      if (itr->i > 0) {
        itr->pos.row -= itr->x->key[itr->i - 1].pos.row;
        itr->pos.col = itr->s[itr->lvl].oldcol;
      }
    }
  } else {
    // we stood at an "internal" key. Go down to the first non-internal
    // key after it.
    while (itr->x->level > 0) {
      // internal key, there is always a child after
      if (itr->i > 0) {
        itr->s[itr->lvl].oldcol = itr->pos.col;
        compose(&itr->pos, itr->x->key[itr->i - 1].pos);
      }
      if (oldbase && itr->i == 0) {
        oldbase[itr->lvl + 1] = oldbase[itr->lvl];
      }
      itr->s[itr->lvl].i = itr->i;
      assert(itr->x->ptr[itr->i]->parent == itr->x);
      itr->lvl++;
      itr->x = itr->x->ptr[itr->i];
      if (preload && itr->x->level) {
        itr->i = -1;
        break;
      } else {
        itr->i = 0;
      }
    }
  }
  return true;
}

bool marktree_itr_prev(MarkTree *b, MarkTreeIter *itr)
{
  if (!itr->x) {
    return false;
  }
  if (itr->x->level == 0) {
    itr->i--;
    if (itr->i >= 0) {
      // TODO(bfredl): this is the common case,
      // and could be handled by inline wrapper
      return true;
    }
    // we ran out of non-internal keys. Go up until we find a non-internal key
    while (itr->i < 0) {
      itr->x = itr->x->parent;
      if (itr->x == NULL) {
        return false;
      }
      itr->lvl--;
      itr->i = itr->s[itr->lvl].i - 1;
      if (itr->i >= 0) {
        itr->pos.row -= itr->x->key[itr->i].pos.row;
        itr->pos.col = itr->s[itr->lvl].oldcol;
      }
    }
  } else {
    // we stood at an "internal" key. Go down to the last non-internal
    // key before it.
    while (itr->x->level > 0) {
      // internal key, there is always a child before
      if (itr->i > 0) {
        itr->s[itr->lvl].oldcol = itr->pos.col;
        compose(&itr->pos, itr->x->key[itr->i - 1].pos);
      }
      itr->s[itr->lvl].i = itr->i;
      assert(itr->x->ptr[itr->i]->parent == itr->x);
      itr->x = itr->x->ptr[itr->i];
      itr->i = itr->x->n;
      itr->lvl++;
    }
    itr->i--;
  }
  return true;
}


bool marktree_itr_node_done(MarkTreeIter *itr)
{
  return !itr->x || itr->i == itr->x->n - 1;
}

mtpos_t marktree_itr_pos(MarkTreeIter *itr)
{
  mtpos_t pos = rawkey(itr).pos;
  unrelative(itr->pos, &pos);
  return pos;
}

mtkey_t marktree_itr_current(MarkTreeIter *itr)
{
  if (itr->x) {
    mtkey_t key = rawkey(itr);
    key.pos = marktree_itr_pos(itr);
    return key;
  }
  return MT_INVALID_KEY;
}

static bool itr_eq(MarkTreeIter *itr1, MarkTreeIter *itr2)
{
  return (&rawkey(itr1) == &rawkey(itr2));
}

bool marktree_itr_get_intersect(MarkTree *b, int row, int col,
                                MarkTreeIter *itr)
{
  if (b->n_keys == 0) {
    itr->x = NULL;
    return false;
  }

  itr->x = b->root;
  itr->i = -1;
  itr->lvl = 0;
  itr->pos = mtpos_t(0, 0);
  itr->intersect_pos = mtpos_t(row, col);
  itr->intersect_idx = 0;
  return true;
}

/// step into the intersected position, returning each intersecting pair
///
/// When intersecting intevals are exhausted, 0 will be returned. `itr`
/// is then valid as an ordinary iterator, as if `marktree_itr_get` was
/// called with the intersected position.
bool marktree_itr_step_intersect(MarkTree *b, MarkTreeIter *itr, mtpair_t *pair)
{
  while (itr->i == -1) {
    if (itr->intersect_idx < kv_size(itr->x->intersect)) {
      uint64_t id = kv_A(itr->x->intersect, itr->intersect_idx++);
      pair->start = marktree_lookup(b, id, NULL);
      mtkey_t end = marktree_lookup(b, id|MARKTREE_END_FLAG, NULL);
      pair->end_pos = end.pos;
      return true;
    }

    if (itr->x->level == 0) {
      itr->s[itr->lvl].i = itr->i = 0;
      break;
    }

    mtkey_t k = { .pos = itr->intersect_pos, .flags = 0 };
    itr->i = marktree_getp_aux(itr->x, k, 0) + 1;

    itr->s[itr->lvl].i = itr->i;
    itr->s[itr->lvl].oldcol = itr->pos.col;

    if (itr->i > 0) {
      compose(&itr->pos, itr->x->key[itr->i-1].pos);
      relative(itr->x->key[itr->i-1].pos, &itr->intersect_pos);
    }
    itr->x = itr->x->ptr[itr->i];
    itr->lvl++;
    itr->i = -1;
    itr->intersect_idx = 0;
  }

  while (itr->i < itr->x->n && pos_less(rawkey(itr).pos, itr->intersect_pos)) {
    mtkey_t k = itr->x->key[itr->i++];
    itr->s[itr->lvl].i = itr->i;
    if (mt_start(k)) {
      pair->start = k;
      unrelative(itr->pos, &pair->start.pos);
      mtkey_t end = marktree_lookup(b, mt_lookup_id(k.ns, k.id, true), NULL);
      pair->end_pos = end.pos;
      return true;  // it's a start!
    }
  }

  while (itr->i < itr->x->n) {
    mtkey_t k = itr->x->key[itr->i++];
    if (mt_end(k)) {
      uint64_t id = mt_lookup_id(k.ns, k.id, false);
      if (id2node(b, id) == itr->x) {
        continue;
      }
      pair->start = marktree_lookup(b, id, NULL);
      pair->end_pos = k.pos;
      unrelative(itr->pos, &pair->end_pos);
      return true; // end of a range which began before us!
    }
  }

  // Foam Boam track (A): the backtrack. Return to intersection position.
  itr->i = itr->s[itr->lvl].i;
  assert(itr-> i >= 0);
  if (itr->i >= itr->x->n) {
    marktree_itr_next(b, itr);
  }

  // either on or after the intersected position, bail out
  return false;
}


static void swap_keys(MarkTree *b, MarkTreeIter *itr1, MarkTreeIter *itr2,
                      DamageList *damage)
{
  if (itr1->x != itr2->x) {
    if (mt_paired(rawkey(itr1))) {
      kvi_push(*damage, ((Damage){ mt_lookup_key(rawkey(itr1)), true, itr1->x, itr2->x,
                                   itr1->i, itr2->i }));
    }
    if (mt_paired(rawkey(itr2))) {
      kvi_push(*damage, ((Damage){ mt_lookup_key(rawkey(itr2)), false, itr2->x, itr1->x,
                                   itr2->i, itr1->i }));
    }
  }

  mtkey_t key1 = rawkey(itr1);
  mtkey_t key2 = rawkey(itr2);
  rawkey(itr1) = key2;
  rawkey(itr1).pos = key1.pos;
  rawkey(itr2) = key1;
  rawkey(itr2).pos = key2.pos;
  refkey(b, itr1->x, itr1->i);
  refkey(b, itr2->x, itr2->i);
}

static int damage_cmp(const void *s1, const void *s2)
{
  Damage *d1 = (Damage *)s1, *d2 = (Damage *)s2;
  assert(d1->id != d2->id);
  return d1->id > d2->id;
}

bool marktree_splice(MarkTree *b, int32_t start_line, int start_col, int old_extent_line,
                     int old_extent_col, int new_extent_line, int new_extent_col)
{
  mtpos_t start = { start_line, start_col };
  mtpos_t old_extent = { old_extent_line, old_extent_col };
  mtpos_t new_extent = { new_extent_line, new_extent_col };

  bool may_delete = (old_extent.row != 0 || old_extent.col != 0);
  bool same_line = old_extent.row == 0 && new_extent.row == 0;
  unrelative(start, &old_extent);
  unrelative(start, &new_extent);
  MarkTreeIter itr[1] = { 0 }, enditr[1] = { 0 };

  mtpos_t oldbase[MT_MAX_DEPTH] = { 0 };

  marktree_itr_get_ext(b, start, itr, false, true, oldbase);
  if (!itr->x) {
    // den e FÄRDIG
    return false;
  }
  mtpos_t delta = { new_extent.row - old_extent.row,
                    new_extent.col - old_extent.col };

  if (may_delete) {
    mtpos_t ipos = marktree_itr_pos(itr);
    if (!pos_leq(old_extent, ipos)
        || (old_extent.row == ipos.row && old_extent.col == ipos.col
            && !mt_right(rawkey(itr)))) {
      marktree_itr_get_ext(b, old_extent, enditr, true, true, NULL);
      assert(enditr->x);
      // "assert" (itr <= enditr)
    } else {
      may_delete = false;
    }
  }

  bool past_right = false;
  bool moved = false;
  DamageList damage;
  kvi_init(damage);

  // Follow the general strategy of messing things up and fix them later
  // "oldbase" carries the information needed to calculate old position of
  // children.
  if (may_delete) {
    while (itr->x && !past_right) {
      mtpos_t loc_start = start;
      mtpos_t loc_old = old_extent;
      relative(itr->pos, &loc_start);

      relative(oldbase[itr->lvl], &loc_old);

continue_same_node:
      // NB: strictly should be less than the right gravity of loc_old, but
      // the iter comparison below will already break on that.
      if (!pos_leq(rawkey(itr).pos, loc_old)) {
        break;
      }

      if (mt_right(rawkey(itr))) {
        while (!itr_eq(itr, enditr)
               && mt_right(rawkey(enditr))) {
          marktree_itr_prev(b, enditr);
        }
        if (!mt_right(rawkey(enditr))) {
          swap_keys(b, itr, enditr, &damage);
        } else {
          past_right = true;  // NOLINT
          (void)past_right;
          break;
        }
      }

      if (itr_eq(itr, enditr)) {
        // actually, will be past_right after this key
        past_right = true;
      }

      moved = true;
      if (itr->x->level) {
        oldbase[itr->lvl + 1] = rawkey(itr).pos;
        unrelative(oldbase[itr->lvl], &oldbase[itr->lvl + 1]);
        rawkey(itr).pos = loc_start;
        marktree_itr_next_skip(b, itr, false, false, oldbase);
      } else {
        rawkey(itr).pos = loc_start;
        if (itr->i < itr->x->n - 1) {
          itr->i++;
          if (!past_right) {
            goto continue_same_node;
          }
        } else {
          marktree_itr_next(b, itr);
        }
      }
    }
    while (itr->x) {
      mtpos_t loc_new = new_extent;
      relative(itr->pos, &loc_new);
      mtpos_t limit = old_extent;

      relative(oldbase[itr->lvl], &limit);

past_continue_same_node:

      if (pos_leq(limit, rawkey(itr).pos)) {
        break;
      }

      mtpos_t oldpos = rawkey(itr).pos;
      rawkey(itr).pos = loc_new;
      moved = true;
      if (itr->x->level) {
        oldbase[itr->lvl + 1] = oldpos;
        unrelative(oldbase[itr->lvl], &oldbase[itr->lvl + 1]);

        marktree_itr_next_skip(b, itr, false, false, oldbase);
      } else {
        if (itr->i < itr->x->n - 1) {
          itr->i++;
          goto past_continue_same_node;
        } else {
          marktree_itr_next(b, itr);
        }
      }
    }
  }

  while (itr->x) {
    unrelative(oldbase[itr->lvl], &rawkey(itr).pos);
    int realrow = rawkey(itr).pos.row;
    assert(realrow >= old_extent.row);
    bool done = false;
    if (realrow == old_extent.row) {
      if (delta.col) {
        rawkey(itr).pos.col += delta.col;

      }
    } else {
      if (same_line) {
        // optimization: column only adjustment can skip remaining rows
        done = true;
      }
    }
    if (delta.row) {
      rawkey(itr).pos.row += delta.row;
      moved = true;
    }
    relative(itr->pos, &rawkey(itr).pos);
    if (done) {
      break;
    }
    marktree_itr_next_skip(b, itr, true, false, NULL);
  }

  if (kv_size(damage)) {
    // TODO(bfredl): a full sort is not really needed. we just need a "start" node to find
    // its corresponding "end" node. Set up some dedicated hash for this later (c.f. the
    // "grow only" variant of khash_t branch)
    qsort((void *)&kv_A(damage, 0), kv_size(damage), sizeof(kv_A(damage, 0)),
          damage_cmp);

    for (size_t i = 0; i < kv_size(damage); i++) {
      Damage d = kv_A(damage, i);
      if (!(d.id & MARKTREE_END_FLAG)) { // start
        if (i+1 < kv_size(damage) && kv_A(damage, i+1).id == (d.id | MARKTREE_END_FLAG)) {
          Damage d2 = kv_A(damage, i+1);

          // pair
          marktree_itr_set_node(b, itr, d.old, d.old_i);
          marktree_itr_set_node(b, enditr, d2.old, d2.old_i);
          marktree_intersect_pair(b, d.id, itr, enditr, true);
          marktree_itr_set_node(b, itr, d.new, d.new_i);
          marktree_itr_set_node(b, enditr, d2.new, d2.new_i);
          marktree_intersect_pair(b, d.id, itr, enditr, false);

          i++; // consume two items
          continue;
        }

        // d is lone start, end didn't move
        MarkTreeIter endpos[1];
        marktree_lookup(b, d.id | MARKTREE_END_FLAG, endpos);
        if (endpos->x) {
          marktree_itr_set_node(b, itr, d.old, d.old_i);
          *enditr = *endpos;
          marktree_intersect_pair(b, d.id, itr, enditr, true);
          marktree_itr_set_node(b, itr, d.new, d.new_i);
          *enditr = *endpos;
          marktree_intersect_pair(b, d.id, itr, enditr, false);
        }
      } else {
        // d is lone end, start didn't move
        MarkTreeIter startpos[1];
        uint64_t start_id = d.id & ~MARKTREE_END_FLAG;

        marktree_lookup(b, start_id, startpos);
        if (startpos->x) {
          *itr = *startpos;
          marktree_itr_set_node(b, enditr, d.old, d.old_i);
          marktree_intersect_pair(b, start_id, itr, enditr, true);
          *itr = *startpos;
          marktree_itr_set_node(b, enditr, d.new, d.new_i);
          marktree_intersect_pair(b, start_id, itr, enditr, false);
        }
      }
    }
  }

  return moved;
}

void marktree_move_region(MarkTree *b, int start_row, colnr_T start_col, int extent_row,
                          colnr_T extent_col, int new_row, colnr_T new_col)
{
  mtpos_t start = { start_row, start_col }, size = { extent_row, extent_col };
  mtpos_t end = size;
  unrelative(start, &end);
  MarkTreeIter itr[1] = { 0 };
  marktree_itr_get_ext(b, start, itr, false, true, NULL);
  kvec_t(mtkey_t) saved = KV_INITIAL_VALUE;
  while (itr->x) {
    mtkey_t k = marktree_itr_current(itr);
    if (!pos_leq(k.pos, end) || (k.pos.row == end.row && k.pos.col == end.col
                                 && mt_right(k))) {
      break;
    }
    relative(start, &k.pos);
    kv_push(saved, k);
    marktree_del_itr(b, itr, false);
  }

  marktree_splice(b, start.row, start.col, size.row, size.col, 0, 0);
  mtpos_t new = { new_row, new_col };
  marktree_splice(b, new.row, new.col,
                  0, 0, size.row, size.col);

  for (size_t i = 0; i < kv_size(saved); i++) {
    mtkey_t item = kv_A(saved, i);
    unrelative(new, &item.pos);
    marktree_put_key(b, item);
    if (mt_paired(item)) {
      // other end might be later in `saved`, this will safely bail out then
      marktree_restore_pair(b, item);
    }
  }
  kv_destroy(saved);
}

/// @param itr OPTIONAL. set itr to pos.
mtkey_t marktree_lookup_ns(MarkTree *b, uint32_t ns, uint32_t id, bool end, MarkTreeIter *itr)
{
  return marktree_lookup(b, mt_lookup_id(ns, id, end), itr);
}

/// @param itr OPTIONAL. set itr to pos.
mtkey_t marktree_lookup(MarkTree *b, uint64_t id, MarkTreeIter *itr)
{
  mtnode_t *n = id2node(b, id);
  if (n == NULL) {
    if (itr) {
      itr->x = NULL;
    }
    return MT_INVALID_KEY;
  }
  int i = 0;
  for (i = 0; i < n->n; i++) {
    if (mt_lookup_key(n->key[i]) == id) {
      return marktree_itr_set_node(b, itr, n, i);
    }
  }

  abort();
}

mtkey_t marktree_itr_set_node(MarkTree *b, MarkTreeIter *itr, mtnode_t *n, int i) {
  mtkey_t key = n->key[i];
  if (itr) {
    itr->i = i;
    itr->x = n;
    itr->lvl = b->root->level - n->level;
  }
  while (n->parent != NULL) {
    mtnode_t *p = n->parent;
    for (i = 0; i < p->n + 1; i++) {
      if (p->ptr[i] == n) {
        goto found_node;
      }
    }
    abort();
found_node:
    if (itr) {
      itr->s[b->root->level - p->level].i = i;
    }
    if (i > 0) {
      unrelative(p->key[i - 1].pos, &key.pos);
    }
    n = p;
  }
  if (itr) {
    marktree_itr_fix_pos(b, itr);
  }
  return key;
}

mtpos_t marktree_get_altpos(MarkTree *b, mtkey_t mark, MarkTreeIter *itr)
{
  return marktree_get_alt(b, mark, itr).pos;
}

mtkey_t marktree_get_alt(MarkTree *b, mtkey_t mark, MarkTreeIter *itr)
{
  mtkey_t end = MT_INVALID_KEY;
  if (mt_paired(mark)) {
    end = marktree_lookup_ns(b, mark.ns, mark.id, !mt_end(mark), itr);
  }
  return end;
}

static void marktree_itr_fix_pos(MarkTree *b, MarkTreeIter *itr)
{
  itr->pos = (mtpos_t){ 0, 0 };
  mtnode_t *x = b->root;
  for (int lvl = 0; lvl < itr->lvl; lvl++) {
    itr->s[lvl].oldcol = itr->pos.col;
    int i = itr->s[lvl].i;
    if (i > 0) {
      compose(&itr->pos, x->key[i - 1].pos);
    }
    assert(x->level);
    x = x->ptr[i];
  }
  assert(x == itr->x);
}

// for unit test
void marktree_put_test(MarkTree *b, uint32_t ns, uint32_t id, int row, int col, bool right_gravity, int end_row, int end_col, bool end_right)
{
  mtkey_t key = { { row, col }, ns, id, 0,
                  mt_flags(right_gravity, 0), 0, NULL };
  marktree_put(b, key, end_row, end_col, end_right);
}

// for unit test
bool mt_right_test(mtkey_t key)
{
  return mt_right(key);
}

// for unit test
void marktree_del_pair_test(MarkTree *b, uint32_t ns, uint32_t id)
{
  MarkTreeIter itr[1];
  marktree_lookup_ns(b, ns, id, false, itr);

  uint64_t other = marktree_del_itr(b, itr, false);
  assert(other);
  marktree_lookup(b, other, itr);
  marktree_del_itr(b, itr, false);
}

void marktree_check(MarkTree *b)
{
#ifndef NDEBUG
  if (b->root == NULL) {
    assert(b->n_keys == 0);
    assert(b->n_nodes == 0);
    assert(b->id2node == NULL || map_size(b->id2node) == 0);
    return;
  }

  mtpos_t dummy;
  bool last_right = false;
  size_t nkeys = marktree_check_node(b, b->root, &dummy, &last_right);
  assert(b->n_keys == nkeys);
  assert(b->n_keys == map_size(b->id2node));
#else
  // Do nothing, as assertions are required
  (void)b;
#endif
}

#ifndef NDEBUG
size_t marktree_check_node(MarkTree *b, mtnode_t *x, mtpos_t *last, bool *last_right)
{
  assert(x->n <= 2 * T - 1);
  // TODO(bfredl): too strict if checking "in repair" post-delete tree.
  assert(x->n >= (x != b->root ? T - 1 : 0));
  size_t n_keys = (size_t)x->n;

  for (int i = 0; i < x->n; i++) {
    if (x->level) {
      n_keys += marktree_check_node(b, x->ptr[i], last, last_right);
    } else {
      *last = (mtpos_t) { 0, 0 };
    }
    if (i > 0) {
      unrelative(x->key[i - 1].pos, last);
    }
    assert(pos_leq(*last, x->key[i].pos));
    if (last->row == x->key[i].pos.row && last->col == x->key[i].pos.col) {
      assert(!*last_right || mt_right(x->key[i]));
    }
    *last_right = mt_right(x->key[i]);
    assert(x->key[i].pos.col >= 0);
    assert(pmap_get(uint64_t)(b->id2node, mt_lookup_key(x->key[i])) == x);
  }

  if (x->level) {
    n_keys += marktree_check_node(b, x->ptr[x->n], last, last_right);
    unrelative(x->key[x->n - 1].pos, last);

    for (int i = 0; i < x->n + 1; i++) {
      assert(x->ptr[i]->parent == x);
      assert(x->ptr[i]->level == x->level - 1);
      // PARANOIA: check no double node ref
      for (int j = 0; j < i; j++) {
        assert(x->ptr[i] != x->ptr[j]);
      }
    }
  } else if (x->n > 0) {
    *last = x->key[x->n - 1].pos;
  }
  return n_keys;
}

bool marktree_check_intersections(MarkTree *b)
{
  if (!b->root) return true;
  PMap(ptr_t) checked = MAP_INIT;

  // 1. move x->intersect to checked[x] and reinit x->intersect
  recurse_nodes(b->root, &checked);

  // 2. iterate over all marks. for each START mark of a pair,
  // intersect the nodes between the pair
  MarkTreeIter itr[1];
  marktree_itr_first(b, itr);
  while (true) {
    mtkey_t mark = marktree_itr_current(itr);
    if (mark.pos.row < 0) {
      break;
    }

    if (mt_start(mark)) {
      MarkTreeIter start_itr[1];
      MarkTreeIter end_itr[1];
      uint64_t end_id = mt_lookup_id(mark.ns, mark.id, true);
      marktree_lookup(b, end_id, end_itr);
      *start_itr = *itr;
      marktree_intersect_pair(b, mt_lookup_key(mark), start_itr, end_itr, false);
    }

    marktree_itr_next(b, itr);
  }

  // 3. for each node check if the recreated intersection
  // matches the old checked[x] intersection.
  bool status = recurse_nodes_compare(b->root, &checked);

  uint64_t *val;
  map_foreach_value(ptr_t, &checked, val, {
    xfree(val);
  });
  map_destroy(ptr_t, &checked);

  return status;
}

static void recurse_nodes(mtnode_t *x, PMap(ptr_t) *checked) {
  if (kv_size(x->intersect)) {
    kvi_push(x->intersect, (uint64_t)-1); // sentinel
    uint64_t *val;
    if (x->intersect.items == x->intersect.init_array) {
      val = xmemdup(x->intersect.items, x->intersect.size*sizeof(*x->intersect.items));
    } else {
      val = x->intersect.items;
    }
    pmap_put(ptr_t)(checked, x, val);
    kvi_init(x->intersect);
  }

  if (x->level) {
    for (int i = 0; i < x->n+1; i++) {
      recurse_nodes(x->ptr[i], checked);
    }
  }
}

static bool recurse_nodes_compare(mtnode_t *x, PMap(ptr_t) *checked) {
  uint64_t *ref = pmap_get(ptr_t)(checked, x);
  if (ref != NULL) {
    for (size_t i = 0;; i++) {
      if (ref[i] == (uint64_t)-1) {
        if (i != kv_size(x->intersect)) {
          return false;
        }

        break;
      } else {
        if (kv_size(x->intersect) <= i || ref[i] != kv_A(x->intersect, i)) {
          return false;
        }
      }
    }
  } else {
    if (kv_size(x->intersect)) {
      return false;
    }
  }

  if (x->level) {
    for (int i = 0; i < x->n+1; i++) {
      if (!recurse_nodes_compare(x->ptr[i], checked)) {
        return false;
      }
    }
  }

  return true;
}

#endif

// TODO(bfredl): kv_print
#define GA_PUT(x) ga_concat(ga, (char *)(x))
#define GA_PRINT(fmt, ...) snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
                           GA_PUT(buf);

String mt_inspect(MarkTree *b, bool keys, bool dot)
{
  garray_T ga[1];
  ga_init(ga, (int)sizeof(char), 80);
  mtpos_t p = { 0, 0 };
  if (b->root) {
    if (dot) {
      GA_PUT("digraph D {\n\n");
      mt_inspect_dotfile_node(b, ga, b->root, p, -1);
      GA_PUT("\n}");
    } else {
      mt_inspect_node(b, ga, keys, b->root, p);
    }
  }
  return ga_take_string(ga);
}

void mt_inspect_node(MarkTree *b, garray_T *ga, bool keys,
                     mtnode_t *n, mtpos_t off)
{
  static char buf[1024];
  GA_PUT("[");
  if (keys && kv_size(n->intersect)) {
    for (size_t i = 0; i < kv_size(n->intersect); i++) {
      GA_PUT(i == 0 ? "{" : ";");
      //GA_PRINT("%"PRIu64, kv_A(n->intersect, i));
      GA_PRINT("%"PRIu64, kv_A(n->intersect, i) & 0xFFFFFFFF);
    }
    GA_PUT("},");
  }
  if (n->level) {
    mt_inspect_node(b, ga, keys, n->ptr[0], off);
  }
  for (int i = 0; i < n->n; i++) {
    mtpos_t p = n->key[i].pos;
    unrelative(off, &p);
    GA_PRINT("%d/%d", p.row, p.col);
    if (keys) {
      mtkey_t key = n->key[i];
      GA_PUT(":");
      if (mt_start(key)) {
        GA_PUT("<");
      }
      // GA_PRINT("%"PRIu64, mt_lookup_id(key.ns, key.id, false));
      GA_PRINT("%"PRIu32, key.id);
      if (mt_end(key)) {
        GA_PUT(">");
      }
    }
    if (n->level) {
      mt_inspect_node(b, ga, keys, n->ptr[i+1], p);
    } else {
      ga_concat(ga, ",");
    }
  }
  ga_concat(ga, "]");
}

void mt_inspect_dotfile_node(MarkTree *b, garray_T *ga,
                     mtnode_t *n, mtpos_t off, int parent)
{
  static char buf[1024];
  int my_id = ga->ga_len;
  GA_PRINT("  N%d[shape=plaintext, label=<\n", my_id);
  GA_PUT("    <table border='0' cellborder='1' cellspacing='0'>\n");
  if (kv_size(n->intersect)) {
    GA_PUT("    <tr><td>");
    for (size_t i = 0; i < kv_size(n->intersect); i++) {
      if (i > 0) {
        GA_PUT(", ");
      }
      GA_PRINT("%"PRIu64, (kv_A(n->intersect, i)>>1) & 0xFFFFFFFF);
    }
    GA_PUT("</td></tr>\n");
  }

  GA_PUT("    <tr><td>");
  for (int i = 0; i < n->n; i++) {
    mtkey_t k = n->key[i];
    if (i > 0) {
      GA_PUT(", ");
    }
    GA_PRINT("%d", k.id);
    if (mt_paired(k)) {
      GA_PUT(mt_end(k) ? "e" : "s");
    }
  }
  GA_PUT("</td></tr>\n");
  GA_PUT("    </table>\n");
  GA_PUT(">];\n");
  if (parent > 0) {
    GA_PRINT("  N%d -> N%d\n", parent, my_id);
  }
  if (n->level) {
    mt_inspect_dotfile_node(b, ga, n->ptr[0], off, my_id);
  }
  for (int i = 0; i < n->n; i++) {
    mtpos_t p = n->key[i].pos;
    unrelative(off, &p);
    if (n->level) {
      mt_inspect_dotfile_node(b, ga, n->ptr[i+1], p, my_id);
    }
  }
}
