#ifndef NVIM_MARK_EXTENDED_H
#define NVIM_MARK_EXTENDED_H

#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"
#include "nvim/map.h"

// TODO(timeyyy): Support '.', normal vim marks etc
#define Extremity -1    // Start or End of lnum or col


#define extline_cmp(a, b) (kb_generic_cmp((a)->lnum, (b)->lnum))

// Macro Documentation: FOR_ALL_?
// Search exclusively using the range values given.
// -1 can be input for range values to the start and end of the line/col

// see FOR_ALL_? for documentation
#define FOR_ALL_EXTMARKLINES(buf, l_lnum, u_lnum, code)\
  kbitr_t(extlines) itr;\
  ExtMarkLine t;\
  t.lnum = l_lnum == Extremity ? MINLNUM : l_lnum;\
  if (!kb_itr_get(extlines, &buf->b_extlines, &t, &itr)) { \
    kb_itr_next(extlines, &buf->b_extlines, &itr);\
  }\
  ExtMarkLine *extline;\
  for (; kb_itr_valid(&itr); kb_itr_next(extlines, &buf->b_extlines, &itr)) { \
    extline = kb_itr_key(&itr);\
    if (extline->lnum > u_lnum && u_lnum != Extremity) { \
      break;\
    }\
      code;\
    }

// see FOR_ALL_? for documentation
#define FOR_ALL_EXTMARKLINES_PREV(buf, l_lnum, u_lnum, code)\
  kbitr_t(extlines) itr;\
  ExtMarkLine t;\
  t.lnum = u_lnum == Extremity ? MAXLNUM : u_lnum;\
  if (!kb_itr_get(extlines, &buf->b_extlines, &t, &itr)) { \
    kb_itr_prev(extlines, &buf->b_extlines, &itr);\
  }\
  ExtMarkLine *extline;\
  for (; kb_itr_valid(&itr); kb_itr_prev(extlines, &buf->b_extlines, &itr)) { \
    extline = kb_itr_key(&itr);\
    if (extline->lnum < l_lnum && l_lnum != Extremity) { \
      break;\
    }\
    code;\
  }

// see FOR_ALL_? for documentation
#define FOR_ALL_EXTMARKS(buf, ns, l_lnum, l_col, u_lnum, u_col, code)\
  kbitr_t(markitems) mitr;\
  ExtendedMark mt;\
  mt.ns_id = ns;\
  mt.mark_id = 0;\
  mt.line = NULL;\
  FOR_ALL_EXTMARKLINES(buf, l_lnum, u_lnum, { \
    mt.col = (l_col ==  Extremity || extline->lnum != l_lnum) ? MINCOL : l_col;\
    if (!kb_itr_get(markitems, &extline->items, mt, &mitr)) { \
        kb_itr_next(markitems, &extline->items, &mitr);\
    } \
    ExtendedMark *extmark;\
    for (; \
         kb_itr_valid(&mitr); \
         kb_itr_next(markitems, &extline->items, &mitr)) { \
      extmark = &kb_itr_key(&mitr);\
      if (u_col != Extremity \
          && extmark->line->lnum == u_lnum \
          && extmark->col > u_col) { \
        break;\
      }\
      code;\
    }\
  })


// see FOR_ALL_? for documentation
#define FOR_ALL_EXTMARKS_PREV(buf, ns, l_lnum, l_col, u_lnum, u_col, code)\
  kbitr_t(markitems) mitr;\
  ExtendedMark mt;\
  mt.mark_id = sizeof(uint64_t);\
  mt.ns_id = ns;\
  FOR_ALL_EXTMARKLINES_PREV(buf, l_lnum, u_lnum, { \
    mt.col = (u_col == Extremity || extline->lnum != u_lnum) ? MAXCOL : u_col;\
    if (!kb_itr_get(markitems, &extline->items, mt, &mitr)) { \
        kb_itr_prev(markitems, &extline->items, &mitr);\
    } \
    ExtendedMark *extmark;\
    for (; \
         kb_itr_valid(&mitr); \
         kb_itr_prev(markitems, &extline->items, &mitr)) { \
      extmark = &kb_itr_key(&mitr);\
      if (l_col != Extremity \
          && extmark->line->lnum == l_lnum \
          && extmark->col < l_col) { \
          break;\
      }\
      code;\
    }\
  })


#define FOR_ALL_EXTMARKS_IN_LINE(items, code)\
  kbitr_t(markitems) mitr;\
  ExtendedMark mt;\
  mt.ns_id = 0;\
  mt.mark_id = 0;\
  mt.line = NULL;\
  mt.col = 0;\
  if (!kb_itr_get(markitems, &items, mt, &mitr)) { \
    kb_itr_next(markitems, &items, &mitr);\
  } \
  ExtendedMark *extmark;\
  for (; kb_itr_valid(&mitr); kb_itr_next(markitems, &items, &mitr)) { \
    extmark = &kb_itr_key(&mitr);\
    code;\
  }\


int mark_cmp(ExtendedMark a, ExtendedMark b);

#define markitems_cmp(a, b) (mark_cmp((a), (b)))
typedef struct kbnode_markitems_s kbnode_markitems_t;
struct kbnode_markitems_s {
    int32_t n;
    _Bool is_internal;
    ExtendedMark key[2 * 10 - 1];
    kbnode_markitems_t *ptr[];
};
typedef struct {
    kbnode_markitems_t *root;
    int n_keys, n_nodes;
} kbtree_markitems_t;
typedef struct {
    kbnode_markitems_t *x;
    int i;
} kbpos_markitems_t;
typedef struct {
    kbpos_markitems_t stack[64], *p;
} kbitr_markitems_t;

static inline int __kb_getp_aux_markitems(const kbnode_markitems_t *__restrict x, ExtendedMark *__restrict k, int *r) {
  int tr, *rr, begin = 0, end = x->n;
  if (x->n == 0)return -1;
  rr = r ? r : &tr;
  while (begin < end) {
    int mid = (begin + end) >> 1;
    if ((mark_cmp(((x->key)[mid]), (*k))) < 0) {
      begin = mid + 1;
    }
    else {
      end = mid;
    }
  }
  if (begin == x->n) {
    *rr = 1;
    return x->n - 1;
  }
  if ((*rr = (mark_cmp((*k), ((x->key)[begin])))) < 0) {
    --begin;
  }
  return begin;
}

static ExtendedMark *kb_getp_markitems(kbtree_markitems_t *b, ExtendedMark *__restrict k) {
  if (!b->root) { return 0; }
  int i, r = 0;
  kbnode_markitems_t *x = b->root;
  while (x) {
    i = __kb_getp_aux_markitems(x, k, &r);
    if (i >= 0 && r == 0)return &(x->key)[i];
    if (x->is_internal == 0)return 0;
    x = (x->ptr)[i + 1];
  }
  return 0;
}

static inline ExtendedMark *kb_get_markitems(kbtree_markitems_t *b, ExtendedMark k) { return kb_getp_markitems(b, &k); }

static inline void kb_intervalp_markitems(kbtree_markitems_t *b, ExtendedMark *__restrict k, ExtendedMark **lower,
                                          ExtendedMark **upper) {
  if (!b->root) { return; }
  int i, r = 0;
  kbnode_markitems_t *x = b->root;
  *lower = *upper = 0;
  while (x) {
    i = __kb_getp_aux_markitems(x, k, &r);
    if (i >= 0 && r == 0) {
      *lower = *upper = &(x->key)[i];
      return;
    }
    if (i >= 0)*lower = &(x->key)[i];
    if (i < x->n - 1)*upper = &(x->key)[i + 1];
    if (x->is_internal == 0)return;
    x = (x->ptr)[i + 1];
  }
}

static inline void kb_interval_markitems(kbtree_markitems_t *b, ExtendedMark k, ExtendedMark **lower,
                                         ExtendedMark **upper) { kb_intervalp_markitems(b, &k, lower, upper); }

static inline void __kb_split_markitems(kbtree_markitems_t *b, kbnode_markitems_t *x, int i, kbnode_markitems_t *y) {
  kbnode_markitems_t *z;
  z = (kbnode_markitems_t *) xcalloc(1, y->is_internal ? (sizeof(kbnode_markitems_t) + (2 * 10) * sizeof(void *))
                                                       : sizeof(kbnode_markitems_t));
  ++b->n_nodes;
  z->is_internal = y->is_internal;
  z->n = 10 - 1;
  memcpy((z->key), &(y->key)[10], sizeof(ExtendedMark) * (10 - 1));
  if (y->is_internal)memcpy((z->ptr), &(y->ptr)[10], sizeof(void *) * 10);
  y->n = 10 - 1;
  memmove(&(x->ptr)[i + 2], &(x->ptr)[i + 1], sizeof(void *) * (unsigned int) (x->n - i));
  (x->ptr)[i + 1] = z;
  memmove(&(x->key)[i + 1], &(x->key)[i], sizeof(ExtendedMark) * (unsigned int) (x->n - i));
  (x->key)[i] = (y->key)[10 - 1];
  ++x->n;
}

static inline ExtendedMark *
__kb_putp_aux_markitems(kbtree_markitems_t *b, kbnode_markitems_t *x, ExtendedMark *__restrict k) {
  int i = x->n - 1;
  ExtendedMark *ret;
  if (x->is_internal == 0) {
    i = __kb_getp_aux_markitems(x, k, 0);
    if (i != x->n - 1)memmove(&(x->key)[i + 2], &(x->key)[i + 1], (unsigned int) (x->n - i - 1) * sizeof(ExtendedMark));
    ret = &(x->key)[i + 1];
    *ret = *k;
    ++x->n;
  }
  else {
    i = __kb_getp_aux_markitems(x, k, 0) + 1;
    if ((x->ptr)[i]->n == 2 * 10 - 1) {
      __kb_split_markitems(b, x, i, (x->ptr)[i]);
      if ((mark_cmp((*k), ((x->key)[i]))) > 0)++i;
    }
    ret = __kb_putp_aux_markitems(b, (x->ptr)[i], k);
  }
  return ret;
}

static inline ExtendedMark *kb_putp_markitems(kbtree_markitems_t *b, ExtendedMark *__restrict k) {
  if (!b->root) {
    b->root = (kbnode_markitems_t *) xcalloc(1, (sizeof(kbnode_markitems_t) + (2 * 10) * sizeof(void *)));
    ++b->n_nodes;
  }
  kbnode_markitems_t *r, *s;
  ++b->n_keys;
  r = b->root;
  if (r->n == 2 * 10 - 1) {
    ++b->n_nodes;
    s = (kbnode_markitems_t *) xcalloc(1, (sizeof(kbnode_markitems_t) + (2 * 10) * sizeof(void *)));
    b->root = s;
    s->is_internal = 1;
    s->n = 0;
    (s->ptr)[0] = r;
    __kb_split_markitems(b, s, 0, r);
    r = s;
  }
  return __kb_putp_aux_markitems(b, r, k);
}

static inline void kb_put_markitems(kbtree_markitems_t *b, ExtendedMark k) { kb_putp_markitems(b, &k); }

static inline ExtendedMark
__kb_delp_aux_markitems(kbtree_markitems_t *b, kbnode_markitems_t *x, ExtendedMark *k, int s) {
  int yn, zn, i, r = 0;
  kbnode_markitems_t *xp, *y, *z;
  ExtendedMark kp;
  if (x == 0)return *k;
  if (s) {
    r = x->is_internal == 0 ? 0 : s == 1 ? 1 : -1;
    i = s == 1 ? x->n - 1 : -1;
  }
  else i = __kb_getp_aux_markitems(x, k, &r);
  if (x->is_internal == 0) {
    if (s == 2)++i;
    kp = (x->key)[i];
    memmove(&(x->key)[i], &(x->key)[i + 1], (unsigned int) (x->n - i - 1) * sizeof(ExtendedMark));
    --x->n;
    return kp;
  }
  if (r == 0) {
    if ((yn = (x->ptr)[i]->n) >= 10) {
      xp = (x->ptr)[i];
      kp = (x->key)[i];
      (x->key)[i] = __kb_delp_aux_markitems(b, xp, 0, 1);
      return kp;
    }
    else if ((zn = (x->ptr)[i + 1]->n) >= 10) {
      xp = (x->ptr)[i + 1];
      kp = (x->key)[i];
      (x->key)[i] = __kb_delp_aux_markitems(b, xp, 0, 2);
      return kp;
    }
    else if (yn == 10 - 1 && zn == 10 - 1) {
      y = (x->ptr)[i];
      z = (x->ptr)[i + 1];
      (y->key)[y->n++] = *k;
      memmove(&(y->key)[y->n], (z->key), (unsigned int) z->n * sizeof(ExtendedMark));
      if (y->is_internal)memmove(&(y->ptr)[y->n], (z->ptr), (unsigned int) (z->n + 1) * sizeof(void *));
      y->n += z->n;
      memmove(&(x->key)[i], &(x->key)[i + 1], (unsigned int) (x->n - i - 1) * sizeof(ExtendedMark));
      memmove(&(x->ptr)[i + 1], &(x->ptr)[i + 2], (unsigned int) (x->n - i - 1) * sizeof(void *));
      --x->n;
      xfree(z);
      return __kb_delp_aux_markitems(b, y, k, s);
    }
  }
  ++i;
  if ((xp = (x->ptr)[i])->n == 10 - 1) {
    if (i > 0 && (y = (x->ptr)[i - 1])->n >= 10) {
      memmove(&(xp->key)[1], (xp->key), (unsigned int) xp->n * sizeof(ExtendedMark));
      if (xp->is_internal)memmove(&(xp->ptr)[1], (xp->ptr), (unsigned int) (xp->n + 1) * sizeof(void *));
      (xp->key)[0] = (x->key)[i - 1];
      (x->key)[i - 1] = (y->key)[y->n - 1];
      if (xp->is_internal)(xp->ptr)[0] = (y->ptr)[y->n];
      --y->n;
      ++xp->n;
    }
    else if (i < x->n && (y = (x->ptr)[i + 1])->n >= 10) {
      (xp->key)[xp->n++] = (x->key)[i];
      (x->key)[i] = (y->key)[0];
      if (xp->is_internal)(xp->ptr)[xp->n] = (y->ptr)[0];
      --y->n;
      memmove((y->key), &(y->key)[1], (unsigned int) y->n * sizeof(ExtendedMark));
      if (y->is_internal)memmove((y->ptr), &(y->ptr)[1], (unsigned int) (y->n + 1) * sizeof(void *));
    }
    else if (i > 0 && (y = (x->ptr)[i - 1])->n == 10 - 1) {
      (y->key)[y->n++] = (x->key)[i - 1];
      memmove(&(y->key)[y->n], (xp->key), (unsigned int) xp->n * sizeof(ExtendedMark));
      if (y->is_internal)memmove(&(y->ptr)[y->n], (xp->ptr), (unsigned int) (xp->n + 1) * sizeof(void *));
      y->n += xp->n;
      memmove(&(x->key)[i - 1], &(x->key)[i], (unsigned int) (x->n - i) * sizeof(ExtendedMark));
      memmove(&(x->ptr)[i], &(x->ptr)[i + 1], (unsigned int) (x->n - i) * sizeof(void *));
      --x->n;
      xfree(xp);
      xp = y;
    }
    else if (i < x->n && (y = (x->ptr)[i + 1])->n == 10 - 1) {
      (xp->key)[xp->n++] = (x->key)[i];
      memmove(&(xp->key)[xp->n], (y->key), (unsigned int) y->n * sizeof(ExtendedMark));
      if (xp->is_internal)memmove(&(xp->ptr)[xp->n], (y->ptr), (unsigned int) (y->n + 1) * sizeof(void *));
      xp->n += y->n;
      memmove(&(x->key)[i], &(x->key)[i + 1], (unsigned int) (x->n - i - 1) * sizeof(ExtendedMark));
      memmove(&(x->ptr)[i + 1], &(x->ptr)[i + 2], (unsigned int) (x->n - i - 1) * sizeof(void *));
      --x->n;
      xfree(y);
    }
  }
  return __kb_delp_aux_markitems(b, xp, k, s);
}

static inline ExtendedMark kb_delp_markitems(kbtree_markitems_t *b, ExtendedMark *__restrict k) {
  kbnode_markitems_t *x;
  ExtendedMark ret;
  ret = __kb_delp_aux_markitems(b, b->root, k, 0);
  --b->n_keys;
  if (b->root->n == 0 && b->root->is_internal) {
    --b->n_nodes;
    x = b->root;
    b->root = (x->ptr)[0];
    xfree(x);
  }
  if (ret.mark_id != k->mark_id) {
    int a =1;
    assert(false);
  }
  return ret;
}

static inline ExtendedMark kb_del_markitems(kbtree_markitems_t *b, ExtendedMark k) { return kb_delp_markitems(b, &k); }

static inline void kb_itr_first_markitems(kbtree_markitems_t *b, kbitr_markitems_t *itr) {
  itr->p = 0;
  if (b->n_keys == 0)return;
  itr->p = itr->stack;
  itr->p->x = b->root;
  itr->p->i = 0;
  while (itr->p->x->is_internal && (itr->p->x->ptr)[0] != 0) {
    kbnode_markitems_t *x = itr->p->x;
    ++itr->p;
    itr->p->x = (x->ptr)[0];
    itr->p->i = 0;
  }
}

static inline int kb_itr_next_markitems(kbtree_markitems_t *b, kbitr_markitems_t *itr) {
  if (itr->p < itr->stack)
    return 0;
  for (;;) {
    ++itr->p->i;
    while (itr->p->x && itr->p->i <= itr->p->x->n) {
      itr->p[1].i = 0;
      itr->p[1].x = itr->p->x->is_internal ? (itr->p->x->ptr)[itr->p->i] : 0;
      ++itr->p;
    }
    --itr->p;
    if (itr->p < itr->stack)return 0;
    if (itr->p->x && itr->p->i < itr->p->x->n)return 1;
  }
}

static inline int kb_itr_prev_markitems(kbtree_markitems_t *b, kbitr_markitems_t *itr) {
  if (itr->p < itr->stack)
    return 0;
  for (;;) {
    while (itr->p->x && itr->p->i >= 0) {
      itr->p[1].x = itr->p->x->is_internal ? (itr->p->x->ptr)[itr->p->i] : 0;
      itr->p[1].i = itr->p[1].x ? itr->p[1].x->n : -1;
      ++itr->p;
    }
    --itr->p;
    if (itr->p < itr->stack)return 0;
    --itr->p->i;
    if (itr->p->x && itr->p->i >= 0)return 1;
  }
}

static inline int kb_itr_getp_markitems(kbtree_markitems_t *b, ExtendedMark *__restrict k, kbitr_markitems_t *itr) {
  if (b->n_keys == 0) {
    itr->p = ((void *) 0);
    return 0;
  }
  int i, r = 0;
  itr->p = itr->stack;
  itr->p->x = b->root;
  while (itr->p->x) {
    i = __kb_getp_aux_markitems(itr->p->x, k, &r);
    itr->p->i = i;
    if (i >= 0 && r == 0)return 1;
    ++itr->p->i;
    itr->p[1].x = itr->p->x->is_internal ? (itr->p->x->ptr)[i + 1] : 0;
    ++itr->p;
  }
  return 0;
}

static inline int kb_itr_get_markitems(kbtree_markitems_t *b, ExtendedMark k, kbitr_markitems_t *itr) {
  return kb_itr_getp_markitems(b, &k, itr);
}

static inline void kb_del_itr_markitems(kbtree_markitems_t *b, kbitr_markitems_t *itr) {
  ExtendedMark k = ((itr)->p->x->key)[(itr)->p->i];
  kb_delp_markitems(b, &k);
  kb_itr_getp_markitems(b, &k, itr);
}


typedef struct ExtMarkLine
{
  linenr_T lnum;
  kbtree_t(markitems) items;
} ExtMarkLine;


KBTREE_INIT(extlines, ExtMarkLine *, extline_cmp, 10)


typedef PMap(uint64_t) IntMap;


typedef struct ExtmarkNs {  // For namespacing extmarks
  IntMap *map;              // For fast lookup
  uint64_t free_id;         // For automatically assigning id's
} ExtmarkNs;


typedef kvec_t(ExtendedMark *) ExtmarkArray;
typedef kvec_t(ExtMarkLine *) ExtlineArray;


// Undo/redo extmarks

typedef enum {
  kExtmarkNOOP,      // Extmarks shouldn't be moved
  kExtmarkUndo,      // Operation should be reversable/undoable
  kExtmarkNoUndo,    // Operation should not be reversable
} ExtmarkOp;


typedef struct {
  linenr_T line1;
  linenr_T line2;
  long amount;
  long amount_after;
} Adjust;

typedef struct {
  linenr_T lnum;
  colnr_T mincol;
  long col_amount;
  long lnum_amount;
} ColAdjust;

typedef struct {
    linenr_T lnum;
    colnr_T mincol;
    colnr_T endcol;
    int eol;
} ColAdjustDelete;

typedef struct {
  linenr_T line1;
  linenr_T line2;
  linenr_T last_line;
  linenr_T dest;
  linenr_T num_lines;
  linenr_T extra;
} AdjustMove;

typedef struct {
  uint64_t ns_id;
  uint64_t mark_id;
  linenr_T lnum;
  colnr_T col;
} ExtmarkSet;

typedef struct {
  uint64_t ns_id;
  uint64_t mark_id;
  linenr_T old_lnum;
  colnr_T old_col;
  linenr_T lnum;
  colnr_T col;
} ExtmarkUpdate;

typedef struct {
  uint64_t ns_id;
  uint64_t mark_id;
  linenr_T lnum;
  colnr_T col;
} ExtmarkCopy;

typedef struct {
  linenr_T l_lnum;
  colnr_T l_col;
  linenr_T u_lnum;
  colnr_T u_col;
  linenr_T p_lnum;
  colnr_T p_col;
} ExtmarkCopyPlace;

typedef struct undo_object ExtmarkUndoObject;

typedef enum {
  kLineAdjust,
  kColAdjust,
  kColAdjustDelete,
  kAdjustMove,
  kExtmarkSet,
  kExtmarkDel,
  kExtmarkUpdate,
  kExtmarkCopy,
  kExtmarkCopyPlace,
} UndoObjectType;

struct undo_object {
  UndoObjectType type;
  union {
    Adjust adjust;
    ColAdjust col_adjust;
    ColAdjustDelete col_adjust_delete;
    AdjustMove move;
    ExtmarkSet set;
    ExtmarkUpdate update;
    ExtmarkCopy copy;
    ExtmarkCopyPlace copy_place;
  } data;
};

typedef kvec_t(ExtmarkUndoObject) extmark_undo_vec_t;

// For doing move of extmarks in substitutions
typedef struct {
  lpos_T startpos;
  lpos_T endpos;
  linenr_T lnum;
  long before_newline_in_pat;
  long after_newline_in_pat;
  long after_newline_in_sub;
  long newline_in_pat;
  long newline_in_sub;
  int sublen;
  // linenr_T l_lnum;
  // colnr_T l_col;
  // linenr_T u_lnum;
  // colnr_T u_col;
  // linenr_T p_lnum;
  // colnr_T p_col;
  long lnum_added;
  lpos_T cm_start;  // start of the match
  lpos_T cm_end;    // end of the match
  int eol;    // end of the match
} ExtmarkSubObject;

typedef kvec_t(ExtmarkSubObject) extmark_sub_vec_t;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_MARK_EXTENDED_H
