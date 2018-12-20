#ifndef NVIM_MARK_EXTENDED_H
#define NVIM_MARK_EXTENDED_H

#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"
#include "nvim/map.h"

#define extline_cmp(a, b) (kb_generic_cmp((a)->lnum, (b)->lnum))

// Macro Documentation: FOR_ALL_?
// Search exclusively using the range values given.
// -1 can be input for range values to the start and end of the line/col.
// The ns parameter: Unless otherwise stated, this is only a starting point
//    for the btree to searched in, the results being itterated over will
//    still contain extmarks from other namespaces.

// see FOR_ALL_? for documentation
#define FOR_ALL_EXTMARKLINES(buf, l_lnum, u_lnum, code)\
  kbitr_t(extlines) itr;\
  ExtMarkLine t;\
  t.lnum = l_lnum;\
  if (!kb_itr_get(extlines, &buf->b_extlines, &t, &itr)) { \
    kb_itr_next(extlines, &buf->b_extlines, &itr);\
  }\
  ExtMarkLine *extline;\
  for (; kb_itr_valid(&itr); kb_itr_next(extlines, &buf->b_extlines, &itr)) { \
    extline = kb_itr_key(&itr);\
    if (extline->lnum > u_lnum) { \
      break;\
    }\
      code;\
    }

// see FOR_ALL_? for documentation
#define FOR_ALL_EXTMARKLINES_PREV(buf, l_lnum, u_lnum, code)\
  kbitr_t(extlines) itr;\
  ExtMarkLine t;\
  t.lnum = u_lnum;\
  if (!kb_itr_get(extlines, &buf->b_extlines, &t, &itr)) { \
    kb_itr_prev(extlines, &buf->b_extlines, &itr);\
  }\
  ExtMarkLine *extline;\
  for (; kb_itr_valid(&itr); kb_itr_prev(extlines, &buf->b_extlines, &itr)) { \
    extline = kb_itr_key(&itr);\
    if (extline->lnum < l_lnum) { \
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
    mt.col = (extline->lnum != l_lnum) ? MINCOL : l_col;\
    if (!kb_itr_get(markitems, &extline->items, mt, &mitr)) { \
        kb_itr_next(markitems, &extline->items, &mitr);\
    } \
    ExtendedMark *extmark;\
    for (; \
         kb_itr_valid(&mitr); \
         kb_itr_next(markitems, &extline->items, &mitr)) { \
      extmark = &kb_itr_key(&mitr);\
      if (extmark->line->lnum == u_lnum \
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
    mt.col = (extline->lnum != u_lnum) ? MAXCOL : u_col;\
    if (!kb_itr_get(markitems, &extline->items, mt, &mitr)) { \
        kb_itr_prev(markitems, &extline->items, &mitr);\
    } \
    ExtendedMark *extmark;\
    for (; \
         kb_itr_valid(&mitr); \
         kb_itr_prev(markitems, &extline->items, &mitr)) { \
      extmark = &kb_itr_key(&mitr);\
      if (extmark->line->lnum == l_lnum \
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


// We only need to compare columns as rows are stored in different trees.
// Marks are ordered by: position, namespace, mark_id
// This improves moving marks but slows down all other use cases (searches)
static inline int extmark_cmp(ExtendedMark a, ExtendedMark b)
{
  int cmp = kb_generic_cmp(a.col, b.col);
  if (cmp != 0) {
    return cmp;
  }
  cmp = kb_generic_cmp(a.ns_id, b.ns_id);
  if (cmp != 0) {
    return cmp;
  }
  return kb_generic_cmp(a.mark_id, b.mark_id);
}


#define markitems_cmp(a, b) (extmark_cmp((a), (b)))
KBTREE_INIT(markitems, ExtendedMark, markitems_cmp, 10)


typedef struct ExtMarkLine
{
  linenr_T lnum;
  kbtree_t(markitems) items;
} ExtMarkLine;


KBTREE_INIT(extlines, ExtMarkLine *, extline_cmp, 10)



typedef struct ExtmarkNs {  // For namespacing extmarks
  PMap(uint64_t) *map;      // For fast lookup
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
  int sublen;
} ExtmarkSubSingle;

// For doing move of extmarks in substitutions
typedef struct {
  lpos_T startpos;
  lpos_T endpos;
  linenr_T lnum;
  linenr_T newline_in_pat;
  linenr_T newline_in_sub;
  linenr_T lnum_added;
  lpos_T cm_start;  // start of the match
  lpos_T cm_end;    // end of the match
  int eol;    // end of the match
} ExtmarkSubMulti;

typedef kvec_t(ExtmarkSubSingle) extmark_sub_single_vec_t;
typedef kvec_t(ExtmarkSubMulti) extmark_sub_multi_vec_t;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_MARK_EXTENDED_H
