// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Implements extended marks for plugins. Each mark exists in a btree of
// lines containing btrees of columns.
//
// The btree provides efficient range lookups.
// A map of pointers to the marks is used for fast lookup by mark id.
//
// Marks are moved by calls to: extmark_col_adjust, extmark_adjust, or
// extmark_col_adjust_delete which are based on col_adjust and mark_adjust from
// mark.c
//
// Undo/Redo of marks is implemented by storing the call arguments to
// extmark_col_adjust or extmark_adjust. The list of arguments
// is applied in extmark_apply_undo. The only case where we have to
// copy extmarks is for the area being effected by a delete.
//
// Marks live in namespaces that allow plugins/users to segregate marks
// from other users.
//
// For possible ideas for efficency improvements see:
// http://blog.atom.io/2015/06/16/optimizing-an-important-atom-primitive.html
// TODO(bfredl): These ideas could be used for an enhanced btree, which
// wouldn't need separate line and column layers.
// Other implementations exist in gtk and tk toolkits.
//
// Deleting marks only happens when explicitly calling extmark_del, deleteing
// over a range of marks will only move the marks. Deleting on a mark will
// leave it in same position unless it is on the EOL of a line.

#include <assert.h>
#include "nvim/vim.h"
#include "charset.h"
#include "nvim/mark_extended.h"
#include "nvim/buffer_updates.h"
#include "nvim/memline.h"
#include "nvim/pos.h"
#include "nvim/globals.h"
#include "nvim/map.h"
#include "nvim/lib/kbtree.h"
#include "nvim/undo.h"
#include "nvim/buffer.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.c.generated.h"
#endif

static ExtmarkNs *buf_ns_ref(buf_T *buf, uint64_t ns_id, bool put) {
  if (!buf->b_extmark_ns) {
    if (!put) {
      return NULL;
    }
    buf->b_extmark_ns = map_new(uint64_t, ExtmarkNs)();
    buf->b_extmark_index = map_new(uint64_t, ExtmarkItem)();
  }

  ExtmarkNs *ns = map_ref(uint64_t, ExtmarkNs)(buf->b_extmark_ns, ns_id, put);
  if (put && ns->map == NULL) {
    ns->map = map_new(uint64_t, uint64_t)();
  }
  return ns;
}


/// Create or update an extmark
///
/// must not be used during iteration!
/// @returns whether a new mark was created
bool extmark_set(buf_T *buf, uint64_t ns_id, uint64_t id,
                int row, colnr_T col, ExtmarkOp op)
{
  ExtmarkNs *ns = buf_ns_ref(buf, ns_id, true);
  mtpos_t old_pos;
  bool moved = false;

  if (id == 0) {
    id = ns->free_id++;
  } else {
    uint64_t old_mark = map_get(uint64_t, uint64_t)(ns->map, id);
    if (old_mark) {
      MarkTreeIter itr[1];
      old_pos = marktree_lookup(buf->b_marktree, old_mark, itr);
      if (old_pos.row == row && old_pos.col == col) {
        return false;  // successful no-op
      }
      marktree_del_itr(buf->b_marktree, itr, false);
      moved = true;
    } else {
      ns->free_id = MAX(ns->free_id, id+1);
    }
  }

  uint64_t mark = marktree_put(buf->b_marktree, row, col, true);
  map_put(uint64_t, ExtmarkItem)(buf->b_extmark_index, mark,
                                 (ExtmarkItem){ ns_id, id });
  map_put(uint64_t, uint64_t)(ns->map, id, mark);

  if (op != kExtmarkNoUndo) {
    if (moved) {
      u_extmark_update(buf, ns_id, id, old_pos.row, old_pos.col,
                       row, col);
    } else {
      u_extmark_set(buf, ns_id, id, row, col, kExtmarkSet);
    }
  }
  return !moved;
}

// Remove an extmark
// Returns 0 on missing id
bool extmark_del(buf_T *buf, uint64_t ns_id, uint64_t id, ExtmarkOp op)
{
  ExtmarkNs *ns = buf_ns_ref(buf, ns_id, false);
  if (!ns) {
    return false;
  }

  uint64_t mark = map_get(uint64_t, uint64_t)(ns->map, id);
  if (!mark) {
    return false;
  }

  MarkTreeIter itr[1];
  mtpos_t pos = marktree_lookup(buf->b_marktree, mark, itr);
  assert(pos.row >= 0);
  if (op != kExtmarkNoUndo) {
    u_extmark_set(buf, ns_id, id, pos.row, pos.col, kExtmarkDel);
  }

  marktree_del_itr(buf->b_marktree, itr, false);
  map_del(uint64_t, uint64_t)(ns->map, id);

  return true;
}

// Free extmarks in a ns between lines
// if ns = 0, it means clear all namespaces
void extmark_clear(buf_T *buf, uint64_t ns_id,
                   int l_row, colnr_T l_col,
                   int u_row, colnr_T u_col,
                   ExtmarkOp undo)
{
  if (!buf->b_extmark_ns) {
    return;
  }

  bool marks_cleared = false;
  // TODO(bfredl): or maybe not, clearing should just purge the marks from
  // history
  if (undo == kExtmarkUndo) {
    // Copy marks that would be effected by clear
    u_extmark_copy(buf, ns_id, l_row, l_col, u_row, u_col);
  }

  bool all_ns = (ns_id == 0);
  ExtmarkNs *ns;
  if (!all_ns) {
    ns = buf_ns_ref(buf, ns_id, false);
    if (!ns) {
      // nothing to do
      return;
    }
  }

  MarkTreeIter itr[1];
  marktree_itr_get(buf->b_marktree, l_row, l_col, itr);
  while (true) {
    mtmark_t mark = marktree_itr_test(itr);
    if (mark.row < 0
        || mark.row > u_row
        || (mark.row == u_row && mark.col > u_col)) {
      break;
    }
    ExtmarkItem item = map_get(uint64_t, ExtmarkItem)(buf->b_extmark_index, mark.id);

    if (item.ns_id == ns_id || all_ns) {
      marks_cleared = true;
      ExtmarkNs *my_ns = all_ns ? buf_ns_ref(buf, item.ns_id, false) : ns;
      map_del(uint64_t, uint64_t)(my_ns->map, item.mark_id);
      map_del(uint64_t, ExtmarkItem)(buf->b_extmark_index, mark.id);
      marktree_del_itr(buf->b_marktree, itr, false);
    } else {
      marktree_itr_next(buf->b_marktree, itr);
    }
  }

  // Record the undo for the actual move
  if (marks_cleared && undo == kExtmarkUndo) {
    // TODO: this statust should just be in the "extmark copy" obj already
    //u_extmark_clear(buf, ns, l_lnum, u_lnum);
  }
}

// Returns the position of marks between a range,
// marks found at the start or end index will be included,
// if upper_lnum or upper_col are negative the buffer
// will be searched to the start, or end
// dir can be set to control the order of the array
// amount = amount of marks to find or -1 for all
ExtmarkArray extmark_get(buf_T *buf, uint64_t ns_id,
                         int l_row, colnr_T l_col,
                         int u_row, colnr_T u_col,
                         int64_t amount, bool reverse)
{
  ExtmarkArray array = KV_INITIAL_VALUE;
  MarkTreeIter itr[1];
  // Find all the marks
  marktree_itr_get_ext(buf->b_marktree, (mtpos_t){ l_row, l_col },
                       itr, reverse, false, NULL);
  int order = reverse ? -1 : 1;
  while ((int64_t)kv_size(array) < amount) {
    mtmark_t mark = marktree_itr_test(itr);
    if (mark.row < 0
        || (mark.row - u_row) * order > 0
        || (mark.row == u_row && (mark.col - u_col) * order > 0)) {
      break;
    }
    ExtmarkItem item = map_get(uint64_t, ExtmarkItem)(buf->b_extmark_index, mark.id);
    if (item.ns_id == ns_id) {
      kv_push(array, ((ExtmarkInfo) { .ns_id = item.ns_id,
                                     .mark_id = item.mark_id,
                                     .row = mark.row, .col = mark.col }));
    }
    if (reverse) {
      marktree_itr_prev(buf->b_marktree, itr);
    } else {
      marktree_itr_next(buf->b_marktree, itr);
    }
  }
  return array;
}

/*
// update the position of an extmark
// to update while iterating pass the markitems itr
static void extmark_update(Extmark *extmark, buf_T *buf,
                           uint64_t ns, uint64_t id,
                           linenr_T lnum, colnr_T col,
                           ExtmarkOp op, kbitr_t(markitems) *mitr)
{
  assert(op != kExtmarkNOOP);
  if (op != kExtmarkNoUndo) {
    u_extmark_update(buf, ns, id, extmark->line->lnum, extmark->col,
                     lnum, col);
  }
  ExtmarkLine *old_line = extmark->line;
  // Move the mark to a new line and update column
  if (old_line->lnum != lnum) {
    ExtmarkLine *ref_line = extmarkline_ref(buf, lnum, true);
    extmark_put(col, id, ref_line, ns);
    // Update the hashmap
    ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
    pmap_put(uint64_t)(ns_obj->map, id, ref_line);
    // Delete old mark
    if (mitr != NULL) {
      kb_del_itr(markitems, &(old_line->items), mitr);
    } else {
      kb_del(markitems, &old_line->items, *extmark);
    }
  // Just update the column
  } else {
    if (mitr != NULL) {
      // The btree stays organized during iteration with kbitr_t
      extmark->col = col;
    } else {
      // Keep the btree in order
      kb_del(markitems, &old_line->items, *extmark);
      extmark_put(col, id, old_line, ns);
    }
  }
}
*/

// Lookup an extmark by id
ExtmarkInfo extmark_from_id(buf_T *buf, uint64_t ns_id, uint64_t id)
{
  ExtmarkNs *ns = buf_ns_ref(buf, ns_id, false);
  ExtmarkInfo ret = { 0, 0, -1, -1 };
  if (!ns) {
    return ret;
  }

  uint64_t mark = map_get(uint64_t, uint64_t)(ns->map, id);
  if (!mark) {
    return ret;
  }

  mtpos_t pos = marktree_lookup(buf->b_marktree, mark, NULL);
  assert(pos.row >= 0);

  // TODO: ExtmarkPos just for this?
  ret.ns_id = ns_id;
  ret.mark_id = id;
  ret.row = pos.row;
  ret.col = pos.col;

  return ret;
}


// free extmarks from the buffer
void extmark_free_all(buf_T *buf)
{
  if (!buf->b_extmark_ns) {
    return;
  }

  uint64_t ns;
  ExtmarkNs ns_obj;

  marktree_clear(buf->b_marktree);

  map_foreach(buf->b_extmark_ns, ns, ns_obj, {
    (void)ns;
    map_free(uint64_t, uint64_t)(ns_obj.map);
  });

  map_free(uint64_t, ExtmarkNs)(buf->b_extmark_ns);
  buf->b_extmark_ns = NULL;

  map_free(uint64_t, ExtmarkItem)(buf->b_extmark_index);
  buf->b_extmark_index = NULL;

  // kv_init called to set pointers to NULL
  //kv_destroy(buf->b_extmark_move_space);
  //kv_init(buf->b_extmark_move_space);
}


// Save info for undo/redo of set marks
static void u_extmark_set(buf_T *buf, uint64_t ns, uint64_t id,
                          int row, colnr_T col, UndoObjectType undo_type)
{
  u_header_T  *uhp = u_force_get_undo_header(buf);
  if (!uhp) {
    return;
  }

  ExtmarkSet set;
  set.ns_id = ns;
  set.mark_id = id;
  set.row = row;
  set.col = col;

  ExtmarkUndoObject undo = { .type = undo_type,
                             .data.set = set };

  kv_push(uhp->uh_extmark, undo);
}

// Save info for undo/redo of deleted marks
static void u_extmark_update(buf_T *buf, uint64_t ns, uint64_t id,
                             int old_row, colnr_T old_col,
                             int row, colnr_T col)
{
  u_header_T  *uhp = u_force_get_undo_header(buf);
  if (!uhp) {
    return;
  }

  ExtmarkUpdate update;
  update.ns_id = ns;
  update.mark_id = id;
  update.old_row = old_row;
  update.old_col = old_col;
  update.row = row;
  update.col = col;

  ExtmarkUndoObject undo = { .type = kExtmarkUpdate,
                             .data.update = update };
  kv_push(uhp->uh_extmark, undo);
}

/*
// Hueristic works only for when the user is typing in insert mode
// - Instead of 1 undo object for each char inserted,
//   we create 1 undo objet for all text inserted before the user hits esc
// Return True if we compacted else False
static bool u_compact_col_adjust(buf_T *buf, linenr_T lnum, colnr_T mincol,
                                 long lnum_amount, long col_amount)
{
  u_header_T  *uhp = u_force_get_undo_header(buf);
  if (!uhp) {
    return false;
  }

  if (kv_size(uhp->uh_extmark) < 1) {
    return false;
  }
  // Check the last action
  ExtmarkUndoObject object = kv_last(uhp->uh_extmark);

  if (object.type != kColAdjust) {
    return false;
  }
  ColAdjust undo = object.data.col_adjust;
  bool compactable = false;

  if (!undo.lnum_amount && !lnum_amount) {
    if (undo.lnum == lnum) {
      if ((undo.mincol + undo.col_amount) >= mincol) {
          compactable = true;
  } } }

  if (!compactable) {
    return false;
  }

  undo.col_amount = undo.col_amount + col_amount;
  ExtmarkUndoObject new_undo = { .type = kColAdjust,
                                 .data.col_adjust = undo };
  kv_last(uhp->uh_extmark) = new_undo;
  return true;
}
*/


// save info to undo/redo a :move
void u_extmark_move(buf_T *buf, linenr_T line1, linenr_T line2,
                    linenr_T last_line, linenr_T dest, linenr_T num_lines,
                    linenr_T extra)
{
  u_header_T  *uhp = u_force_get_undo_header(buf);
  if (!uhp) {
    return;
  }

  AdjustMove move;
  move.line1 = line1;
  move.line2 = line2;
  move.last_line = last_line;
  move.dest = dest;
  move.num_lines = num_lines;
  move.extra = extra;

  ExtmarkUndoObject undo = { .type = kAdjustMove,
                             .data.move = move };

  kv_push(uhp->uh_extmark, undo);
}

// copy extmarks data between range, useful when we cannot simply reverse
// the operation. This will do nothing on redo, enforces correct position when
// undo.
// if ns = 0, it means copy all namespaces
void u_extmark_copy(buf_T *buf, uint64_t ns_id,
                    int l_row, colnr_T l_col,
                    int u_row, colnr_T u_col)
{
  u_header_T  *uhp = u_force_get_undo_header(buf);
  if (!uhp) {
    return;
  }

  bool all_ns = (ns_id == 0);

  ExtmarkInfo copy;
  ExtmarkUndoObject undo;

  MarkTreeIter itr[1];
  marktree_itr_get(buf->b_marktree, l_row, l_col, itr);
  while (true) {
    mtmark_t mark = marktree_itr_test(itr);
    if (mark.row < 0
        || mark.row > u_row
        || (mark.row == u_row && mark.col > u_col)) {
      break;
    }
    ExtmarkItem item = map_get(uint64_t, ExtmarkItem)(buf->b_extmark_index, mark.id);
    if (all_ns || item.ns_id == ns_id) {
      copy.ns_id = item.ns_id;
      copy.mark_id = item.mark_id;
      copy.row = mark.row;
      copy.col = mark.col;

      undo.data.copy = copy;
      undo.type = kExtmarkCopy;
      kv_push(uhp->uh_extmark, undo);
    }
    marktree_itr_next(buf->b_marktree, itr);
  }
}

void u_extmark_copy_place(buf_T *buf,
                          linenr_T l_lnum, colnr_T l_col,
                          linenr_T u_lnum, colnr_T u_col,
                          linenr_T p_lnum, colnr_T p_col)
{
  u_header_T  *uhp = u_force_get_undo_header(buf);
  if (!uhp) {
    return;
  }

  ExtmarkCopyPlace copy_place;
  copy_place.l_lnum = l_lnum;
  copy_place.l_col = l_col;
  copy_place.u_lnum = u_lnum;
  copy_place.u_col = u_col;
  copy_place.p_lnum = p_lnum;
  copy_place.p_col = p_col;

  ExtmarkUndoObject undo = { .type = kExtmarkCopyPlace,
                             .data.copy_place = copy_place };

  kv_push(uhp->uh_extmark, undo);
}

// undo or redo an extmark operation
void extmark_apply_undo(ExtmarkUndoObject undo_info, bool undo)
{
  // splice: any text operation changing position (except :move)
  if (undo_info.type == kSplice) {
    // Undo
    ExtmarkSplice splice = undo_info.data.splice;
    if (undo) {
      extmark_splice(curbuf,
                     splice.start_row, splice.start_col,
                     splice.newextent_row, splice.newextent_col,
                     splice.oldextent_row, splice.oldextent_col,
                     kExtmarkNoUndo);

    } else {
      extmark_splice(curbuf,
                     splice.start_row, splice.start_col,
                     splice.oldextent_row, splice.oldextent_col,
                     splice.newextent_row, splice.newextent_col,
                     kExtmarkNoUndo);
    }
  // kExtmarkCopy
  } else if (undo_info.type == kExtmarkCopy) {
    // Redo should be handled by kColAdjustDelete or kExtmarkCopyPlace
    if (undo) {
      extmark_set(curbuf,
                  undo_info.data.copy.ns_id,
                  undo_info.data.copy.mark_id,
                  undo_info.data.copy.row,
                  undo_info.data.copy.col,
                  kExtmarkNoUndo);
    }
  // uses extmark_copy_and_place
  } else if (undo_info.type == kExtmarkCopyPlace) {
    // Redo, undo is handle by kExtmarkCopy
    if (!undo) {
      abort();
      /*
      extmark_copy_and_place(curbuf,
                             undo_info.data.copy_place.l_lnum,
                             undo_info.data.copy_place.l_col,
                             undo_info.data.copy_place.u_lnum,
                             undo_info.data.copy_place.u_col,
                             undo_info.data.copy_place.p_lnum,
                             undo_info.data.copy_place.p_col,
                             kExtmarkNoUndo, true, NULL);
                             */
    }
  // kExtmarkClear
  } else if (undo_info.type == kExtmarkClear) {
    // Redo, undo is handle by kExtmarkCopy
  // kAdjustMove
  } else if (undo_info.type == kAdjustMove) {
    apply_undo_move(undo_info, undo);
  // extmark_set
  } else if (undo_info.type == kExtmarkSet) {
    if (undo) {
      extmark_del(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  kExtmarkNoUndo);
    // Redo
    } else {
      extmark_set(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  undo_info.data.set.row,
                  undo_info.data.set.col,
                  kExtmarkNoUndo);
    }
  // extmark_set into update
  } else if (undo_info.type == kExtmarkUpdate) {
    if (undo) {
      extmark_set(curbuf,
                  undo_info.data.update.ns_id,
                  undo_info.data.update.mark_id,
                  undo_info.data.update.old_row,
                  undo_info.data.update.old_col,
                  kExtmarkNoUndo);
    // Redo
    } else {
      extmark_set(curbuf,
                  undo_info.data.update.ns_id,
                  undo_info.data.update.mark_id,
                  undo_info.data.update.row,
                  undo_info.data.update.col,
                  kExtmarkNoUndo);
    }
  // extmark_del
  } else if (undo_info.type == kExtmarkDel)  {
    if (undo) {
      extmark_set(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  undo_info.data.set.row,
                  undo_info.data.set.col,
                  kExtmarkNoUndo);
    // Redo
    } else {
      extmark_del(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  kExtmarkNoUndo);
    }
  }
}

// undo/redo an kExtmarkMove operation
static void apply_undo_move(ExtmarkUndoObject undo_info, bool undo)
{
  /*
  // 3 calls are required , see comment in function do_move (ex_cmds.c)
  linenr_T line1 = undo_info.data.move.line1;
  linenr_T line2 = undo_info.data.move.line2;
  linenr_T last_line = undo_info.data.move.last_line;
  linenr_T dest = undo_info.data.move.dest;
  linenr_T num_lines = undo_info.data.move.num_lines;
  linenr_T extra = undo_info.data.move.extra;

  if (undo) {
    if (dest >= line2) {
      extmark_adjust(curbuf, dest - num_lines + 1, dest,
                     last_line - dest + num_lines - 1, 0L, kExtmarkNoUndo,
                     true);
      extmark_adjust(curbuf, dest - line2, dest - line1,
                     dest - line2, 0L, kExtmarkNoUndo, false);
    } else {
      extmark_adjust(curbuf, line1-num_lines, line2-num_lines,
                     last_line - (line1-num_lines), 0L, kExtmarkNoUndo, true);
      extmark_adjust(curbuf, (line1-num_lines) + 1, (line2-num_lines) + 1,
                     -num_lines, 0L, kExtmarkNoUndo, false);
    }
    extmark_adjust(curbuf, last_line, last_line + num_lines - 1,
                   line1 - last_line, 0L, kExtmarkNoUndo, true);
  // redo
  } else {
    extmark_adjust(curbuf, line1, line2,
                   last_line - line2, 0L, kExtmarkNoUndo, true);
    if (dest >= line2) {
      extmark_adjust(curbuf, line2 + 1, dest,
                     -num_lines, 0L, kExtmarkNoUndo, false);
    } else {
      extmark_adjust(curbuf, dest + 1, line1 - 1,
                     num_lines, 0L, kExtmarkNoUndo, false);
    }
  extmark_adjust(curbuf, last_line - num_lines + 1, last_line,
                 -(last_line - dest - extra), 0L, kExtmarkNoUndo, true);
  }
  */
}


/// Get the column position for EOL on a line
///
/// If the lnum doesn't exist, returns 0
colnr_T extmark_eol_col(buf_T *buf, linenr_T lnum)
{
  if (lnum > buf->b_ml.ml_line_count) {
    return 0;
  }
  return (colnr_T)STRLEN(ml_get_buf(buf, lnum, false)) + 1;
}



// Adjust columns and rows for extmarks
//
// based off mark_col_adjust in mark.c
// use extmark_col_adjust_impl to move columns by inserting
// Doesn't take the eol into consideration (possible to put marks in invalid
// positions)
void extmark_col_adjust(buf_T *buf, linenr_T lnum,
                        colnr_T mincol, long lnum_amount,
                        long col_amount, ExtmarkOp undo)
{
  assert(col_amount > INT_MIN && col_amount <= INT_MAX);

  // TODO: this check should be when mark_col_adjust calls us, otherwise
  // redundant?
  if (curbuf_splice_pending) {
   return;
  }
  if (lnum_amount || col_amount < 0) {
    // abort();
    return;
  }
  extmark_splice(buf,
                 (int)lnum-1, mincol,
                 0, 0,
                 0, (int)col_amount, undo);
}

// Adjust marks after a delete on a line
//
// Automatically readjusts to take the eol into account
// TODO(timeyyy): change mincol to be for the mark to be copied, not moved
//
// @param mincol First column that needs to be moved (start of delete range) + 1
// @param endcol Last column which needs to be copied (end of delete range + 1)
void extmark_col_adjust_delete(buf_T *buf, linenr_T lnum,
                               colnr_T mincol, colnr_T endcol,
                               ExtmarkOp undo, int _eol)
{
 // colnr_T start_effected_range = mincol;

  // Deletes at the end of the line have different behaviour than the normal
  // case when deleted.
  // Cleanup any marks that are floating beyond the end of line.
  // we allow this to be passed in as well because the buffer may have already
  // been mutated.
  // TODO: this should not be needed, the proper "splice" op should
  // adjust floating marks anyway
  //int eol = _eol;
  //if (!eol) {
  //  eol = extmark_eol_col(buf, lnum);
  //}
  //FOR_ALL_EXTMARKS(buf, 1, lnum, eol, lnum, -1, {
  //  extmark_update(extmark, buf, extmark->ns_id, extmark->mark_id,
  //                 extmarkline->lnum, (colnr_T)eol, kExtmarkNoUndo, &mitr);
  //})

  // Record the undo for the actual move

  if (!curbuf_splice_pending) {
    int old_extent = endcol-mincol+1;
    extmark_splice(buf,
                   (int)lnum-1, mincol-1,
                   0, old_extent,
                   0, 0, undo);
  }
}

// Adjust extmark row for inserted/deleted rows (columns stay fixed).
void extmark_adjust(buf_T *buf,
                    linenr_T line1,
                    linenr_T line2,
                    long amount,
                    long amount_after,
                    ExtmarkOp undo,
                    bool end_temp)
{

  // btree needs to be kept ordered to work, so far only :move requires this
  // 2nd call with end_temp = true unpack the lines from the temp position
  if (end_temp && amount < 0) {
    //for (size_t i = 0; i < kv_size(buf->b_extmark_move_space); i++) {
    //  _extline = kv_A(buf->b_extmark_move_space, i);
    //  _extline->lnum += amount;
    //  kb_put(extmarklines, &buf->b_extlines, _extline);
    //}
    //kv_size(buf->b_extmark_move_space) = 0;
    abort();
    return;
  }

  if (!curbuf_splice_pending) {
    int old_extent, new_extent;
    if (amount == MAXLNUM) {
      old_extent = (int)(line2 - line1+1);
      new_extent = (int)(amount_after + old_extent);
    } else {
      // TODO: handle :move separately
      // assert(line2 == MAXLNUM);
      old_extent = 0;
      new_extent = (int)amount;
    }
    extmark_splice(buf,
                   (int)line1-1, 0,
                   old_extent, 0,
                   new_extent, 0, undo);
  }
}

void extmark_splice(buf_T *buf,
                    int start_row, colnr_T start_col,
                    int oldextent_row, colnr_T oldextent_col,
                    int newextent_row, colnr_T newextent_col,
                    ExtmarkOp undo)
{
  buf_updates_send_splice(buf, start_row, start_col,
                          oldextent_row, oldextent_col,
                          newextent_row, newextent_col);

  if (undo == kExtmarkUndo && (oldextent_row > 0 || oldextent_col > 0)) {
    // Copy marks that would be effected by delete
    // TODO: be "smart" about gravity here, left-gravity at the begining
    // and right-gravity at the end need not be preserved
    int end_row = start_row + oldextent_row;
    int end_col = (oldextent_row ? 0 : start_col) + oldextent_col;
    u_extmark_copy(buf, 0, start_row, start_col, end_row, end_col);
  }


  bool marks_moved = marktree_splice(buf->b_marktree, start_row, start_col,
                                     oldextent_row, oldextent_col,
                                     newextent_row, newextent_col);

  if (undo == kExtmarkUndo && marks_moved) {
    u_header_T  *uhp = u_force_get_undo_header(buf);
    if (!uhp) {
      return;
    }

    //if (!u_compact_col_adjust(buf, lnum, mincol, lnum_amount, col_amount)) {
    if (true) {
      ExtmarkSplice splice;
      splice.start_row = start_row;
      splice.start_col = start_col;
      splice.oldextent_row = oldextent_row;
      splice.oldextent_col = oldextent_col;
      splice.newextent_row = newextent_row;
      splice.newextent_col = newextent_col;

      kv_push(uhp->uh_extmark,
              ((ExtmarkUndoObject){ .type = kSplice,
                                    .data.splice = splice }));
    }
  }
}

/*
/// Range points to copy
///
/// if part of a larger iteration we can't delete, then the caller
/// must check for empty lines.
bool extmark_copy_and_place(buf_T *buf,
                            linenr_T l_lnum, colnr_T l_col,
                            linenr_T u_lnum, colnr_T u_col,
                            linenr_T p_lnum, colnr_T p_col,
                            ExtmarkOp undo, bool delete,
                            ExtmarkLine *destline)

{
  bool marks_moved = false;
  if (undo == kExtmarkUndo || undo == kExtmarkUndoNoRedo) {
    // Copy marks that would be effected by delete
    u_extmark_copy(buf, 0, l_lnum, l_col, u_lnum, u_col);
  }

  // Move extmarks to their final position
  // Careful: if we move items within the same line, we might change order of
  // marks within the same extmarkline. Too keep it simple, first delete all
  // items from the extmarkline and put them back in the right order.
  FOR_ALL_EXTMARKLINES(buf, l_lnum, u_lnum, {
    kvec_t(Extmark) temp_space = KV_INITIAL_VALUE;
    bool same_line = extmarkline == destline;
    FOR_ALL_EXTMARKS_IN_LINE(extmarkline->items,
                             (extmarkline->lnum > l_lnum) ? 0 : l_col,
                             (extmarkline->lnum < u_lnum) ? MAXCOL : u_col, {
      if (!destline) {
        destline = extmarkline_ref(buf, p_lnum, true);
        same_line = extmarkline == destline;
      }
      marks_moved = true;
      if (!same_line) {
        extmark_put(p_col, extmark->mark_id, destline, extmark->ns_id);
        ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns,
                                               extmark->ns_id);
        pmap_put(uint64_t)(ns_obj->map, extmark->mark_id, destline);
      } else {
        kv_push(temp_space, *extmark);
      }
      // Delete old mark
      kb_del_itr(markitems, &extmarkline->items, &mitr);
    })
    if (same_line) {
      for (size_t i = 0; i < kv_size(temp_space); i++) {
        Extmark mark = kv_A(temp_space, i);
        extmark_put(p_col, mark.mark_id, extmarkline, mark.ns_id);
      }
      kv_destroy(temp_space);
    } else if (delete && kb_size(&extmarkline->items) == 0) {
      kb_del_itr(extmarklines, &buf->b_extlines, &itr);
      extmarkline_free(extmarkline);
    }
  })

  // Record the undo for the actual move
  if (marks_moved && undo == kExtmarkUndo) {
    u_extmark_copy_place(buf, l_lnum, l_col, u_lnum, u_col, p_lnum, p_col);
  }

  return marks_moved;
}
*/


