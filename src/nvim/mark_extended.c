
// mark_extended.c --
// Implements extended marks for text widgets.
// Each Mark exists in a btree of lines containing btrees
// of columns.
//
// The btree provides efficent range lookus.
// A map of pointers to the marks is used for fast lookup by mark id.
//
// Marks are moved by calls to extmark_col_adjust or
// extmark_adjust which are based on col_adjust and mark_adjust from mark.c
//
// Undo/Redo of marks is implemented by storing the call arguments to
// extmark_col_adjust or extmark_adjust. The list of arguments
// is traversed in extmark_iter_undo and applied in extmark_apply_undo
//
// For possible ideas for efficency improvements see:
// http://blog.atom.io/2015/06/16/optimizing-an-important-atom-primitive.html
// Other implementations exist in gtk and tk toolkits.

#include <assert.h>

#include "nvim/vim.h"
#include "nvim/mark_extended.h"
#include "nvim/memory.h"
#include "nvim/pos.h"          // MAXLNUM
#include "nvim/globals.h"      // FOR_ALL_BUFFERS
#include "nvim/map.h"          // pmap ...
#include "nvim/lib/kbtree.h"   // kbitr ...
#include "nvim/undo_defs.h"    // u_header_T
#include "nvim/undo.h"         // u_save_cursor

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.c.generated.h"
#endif

static uint64_t namespace_counter = 0;

// Required before calling any other functions
uint64_t extmark_ns_create(char *ns)
{
  if (!namespace_counter) {
    EXTMARK_NAMESPACES = map_new(uint64_t, cstr_t)();
  }
  // Ensure the namespace is unique
  cstr_t value;
  map_foreach_value(EXTMARK_NAMESPACES, value, {
    if (STRCMP(ns, value) == 0) {
      return 0;
    }
  });
  namespace_counter++;
  uint64_t id = namespace_counter;
  map_put(uint64_t, cstr_t)(EXTMARK_NAMESPACES, id, xstrdup(ns));
  return namespace_counter;
}

bool ns_initialized(uint64_t ns) {
  if (!EXTMARK_NAMESPACES) {
    return 0;
  }
  return map_has(uint64_t, cstr_t)(EXTMARK_NAMESPACES, ns);
}

// TODO(timeyyy): currently possible to set marks where there is no text...
// Create or update an extmark
// Returns 1 on new mark created
// Returns 2 on succesful update
int extmark_set(buf_T *buf,
                uint64_t ns,
                uint64_t id,
                linenr_T lnum,
                colnr_T col,
                ExtmarkReverseType undo)
{
  ExtendedMark *extmark = extmark_from_id(buf, ns, id);
  if (!extmark) {
    return extmark_create(buf, ns, id, lnum, col, undo);
  } else {
    extmark_update(extmark, buf, ns, id, lnum,  col, undo);
    return 2;
  }
}

// Returns 0 on missing id
int extmark_unset(buf_T *buf,
                  uint64_t ns,
                  uint64_t id,
                  ExtmarkReverseType undo)
{
  ExtendedMark *extmark = extmark_from_id(buf, ns, id);
  if (!extmark) {
    return 0;
  }
  return extmark_delete(extmark, buf, ns, id, undo);
}

// Given a mark, finds the next mark
ExtendedMark *extmark_next(buf_T *buf,
                           uint64_t ns,
                           linenr_T lnum,
                           colnr_T col,
                           bool match)
{
  return extmark_neighbour(buf, ns, lnum, col, FORWARD, match);
}

// Given a id finds the previous mark
ExtendedMark *extmark_prev(buf_T *buf,
                           uint64_t ns,
                           linenr_T lnum,
                           colnr_T col,
                           bool match)
{
  return extmark_neighbour(buf, ns, lnum, col, BACKWARD, match);
}

ExtmarkArray *extmark_nextrange(buf_T *buf,
                                uint64_t ns,
                                linenr_T l_lnum,
                                colnr_T l_col,
                                linenr_T u_lnum,
                                colnr_T u_col)
{
  return extmark_neighbour_range(buf,
                                 ns, l_lnum, l_col, u_lnum, u_col, FORWARD);
}

ExtmarkArray *extmark_prevrange(buf_T *buf,
                                uint64_t ns,
                                linenr_T l_lnum,
                                colnr_T l_col,
                                linenr_T u_lnum,
                                colnr_T u_col)
{
  return extmark_neighbour_range(buf,
                                 ns, l_lnum, l_col, u_lnum, u_col, BACKWARD);
}

// Returns the position of marks between a range,
// marks found at the start or end index will be included,
// if upper_lnum or upper_col are negative the buffer
// will be searched to the start, or end
// dir can be set to control the order of the array
static ExtmarkArray *extmark_neighbour_range(buf_T *buf, uint64_t ns,
                                             linenr_T l_lnum, colnr_T l_col,
                                             linenr_T u_lnum, colnr_T u_col,
                                             int dir)
{
  ExtmarkArray *array = xmalloc(sizeof(ExtmarkArray));
  kv_init(*array);
  if (dir == FORWARD) {
    FOR_ALL_EXTMARKS(buf, ns, l_lnum, l_col, u_lnum, u_col, {
      if (extmark->ns_id == ns) {
        kv_push(*array, extmark);
      }
    })
  } else {
    FOR_ALL_EXTMARKS_PREV(buf, ns, l_lnum, l_col, u_lnum, u_col, {
      if (extmark->ns_id == ns) {
        kv_push(*array, extmark);
      }
    })
  }
  return array;
}

// TODO(timeyyy): Not using match param, delete it?

/// Returns the mark, nearest to or including the passed in mark
/// @param dir, flag to control next or prev
/// @param match, a flag to include a mark that is at the postiion being queried
static ExtendedMark *extmark_neighbour(buf_T *buf, uint64_t ns, linenr_T lnum,
                                       colnr_T col, int dir, bool match)
{
  if (dir == FORWARD) {
    FOR_ALL_EXTMARKS(buf, ns, lnum, col, lnum, -1, {
      if (extmark->ns_id == ns) {
        if (extmark->col > col) {
          return extmark;
        } else if (match && extmark->col == col) {
          return extmark;
        }
      }
    })
  } else {
    FOR_ALL_EXTMARKS_PREV(buf, ns, lnum, 0, lnum, col, {
      if (extmark->ns_id == ns) {
        if (extmark->col < col) {
          return extmark;
        } else if (match && extmark->col == col) {
          return extmark;
        }
      }
    })
  }
  return NULL;
}

static bool extmark_create(buf_T *buf,
                           uint64_t ns,
                           uint64_t id,
                           linenr_T lnum,
                           colnr_T col,
                           ExtmarkReverseType undo)
{
  if (!buf->b_extmark_ns) {
    buf->b_extmark_ns = pmap_new(uint64_t)();
  }
  ExtmarkNs *ns_obj = NULL;
  ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
  // Initialize a new namespace for this buffer
  if (!ns_obj) {
    ns_obj = xmalloc(sizeof(ExtmarkNs));
    ns_obj->map = pmap_new(uint64_t)();
    pmap_put(uint64_t)(buf->b_extmark_ns, ns, ns_obj);
  }

  ExtMarkLine *extline = extline_ref(&buf->b_extlines, lnum);
  extmark_put(&(extline->items), col, id, extline, ns);

  // Marks do not have stable address so we have to look them up
  // by using the line instead of the mark
  pmap_put(uint64_t)(ns_obj->map, id, extline);
  if (undo != extmarkNoUndo) {
    u_extmark_set(buf, ns, id, lnum, col, kExtmarkSet);
  }
  return true;
}

static void extmark_update(ExtendedMark *extmark,
                           buf_T *buf,
                           uint64_t ns,
                           uint64_t id,
                           linenr_T lnum,
                           colnr_T col,
                           ExtmarkReverseType undo)
{
  if (undo != extmarkNoUndo) {
    u_extmark_update(buf, ns, id, extmark->line->lnum, extmark->col,
                     lnum, col);
  }
  extmark->col = col;
  extmark->line->lnum = lnum;
}

static int extmark_delete(ExtendedMark *extmark,
                          buf_T *buf,
                          uint64_t ns,
                          uint64_t id,
                          ExtmarkReverseType undo)
{
  if (undo != extmarkNoUndo) {
    u_extmark_set(buf, ns, id, extmark->line->lnum, extmark->col, kExtmarkUnset);
  }

  // Remove our key from the namespace
  ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
  pmap_del(uint64_t)(ns_obj->map, id);

  // Remove the mark mark from the line
  ExtMarkLine *extline = extmark->line;
  kb_del(markitems, &extline->items, *extmark);
  // Remove the line if there are no more marks in the line
  if (kb_size(&extline->items) == 0) {
    kb_del(extlines, &buf->b_extlines, extline);
  }
  return true;
}

ExtendedMark *extmark_from_id(buf_T *buf, uint64_t ns, uint64_t id)
{
  if (!buf->b_extmark_ns) {
    return NULL;
  }
  ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
  if (!ns_obj || !kh_size(ns_obj->map->table)) {
    return NULL;
  }
  ExtMarkLine *extline = pmap_get(uint64_t)(ns_obj->map, id);
  if (!extline) {
    return NULL;
  }

  FOR_ALL_EXTMARKS_IN_LINE(&extline->items, {
    if (extmark->ns_id == ns
        && extmark->mark_id == id) {
      return extmark;
    }
  })
  return NULL;
}

ExtendedMark *extmark_from_pos(buf_T *buf,
                               uint64_t ns, linenr_T lnum, colnr_T col)
{
  if (!buf->b_extmark_ns) {
    return NULL;
  }
  FOR_ALL_EXTMARKS(buf, ns, lnum, col, lnum, col, {
    if (extmark->ns_id == ns) {
      if (extmark->col == col) {
        return extmark;
      }
    }
  })
  return NULL;
}

void extmark_free_all(buf_T *buf)
{
  if (!buf->b_extmark_ns) {
    return;
  }

  uint64_t ns;
  ExtmarkNs *ns_obj;

  map_foreach(buf->b_extmark_ns, ns, ns_obj, {
     FOR_ALL_EXTMARKS(buf, ns, 0, 0, -1, -1, {
       kb_del_itr(markitems, &extline->items, &mitr);
     });
    pmap_free(uint64_t)(ns_obj->map);
    xfree(ns_obj);
  });

  pmap_free(uint64_t)(buf->b_extmark_ns);

  FOR_ALL_EXTMARKLINES(buf, 0, -1, {
    kb_destroy(markitems, (&extline->items));
    kb_del_itr(extlines, &buf->b_extlines, &itr);
    xfree(extline);
  })
  // TODO(timeyyy): why do we need the parans on the 2nd arg?
  kb_destroy(extlines, (&buf->b_extlines));

  // This is emptied after a move
  kv_destroy(buf->b_extmark_end_temp_space);
}

// TODO(timeyyy): make this non static ..
static u_header_T *get_undo_header(buf_T *buf)
{
  u_header_T *uhp = NULL;
  if (buf->b_u_curhead != NULL) {
    uhp = buf->b_u_curhead;
  } else if (buf->b_u_newhead) {
    uhp = buf->b_u_newhead;
  }
  // Create the first undo header for the buffer
  if (!uhp) {
    // TODO(timeyyy): there would be a better way to do this!
    u_save_cursor();
    uhp = buf->b_u_curhead;
    if (!uhp) {
      uhp = buf->b_u_newhead;
      assert(uhp);
    }
  }
  return uhp;
}

// Save info for undo/redo of set, unset marks
static void u_extmark_set(buf_T *buf,
                          uint64_t ns,
                          uint64_t id,
                          linenr_T lnum,
                          colnr_T col,
                          UndoObjectType undo_type)
{
  u_header_T  *uhp = get_undo_header(buf);

  ExtmarkSet set;
  set.ns_id = ns;
  set.mark_id = id;
  set.lnum = lnum;
  set.col = col;

  ExtmarkUndoObject undo = { .type = undo_type,
                             .reverse = extmarkNoReverse,
                             .data.set = set };
  kv_push(uhp->uh_extmark, undo);
}

static void u_extmark_update(buf_T *buf,
                             uint64_t ns,
                             uint64_t id,
                             linenr_T old_lnum,
                             colnr_T old_col,
                             linenr_T lnum,
                             colnr_T col)
{
  u_header_T  *uhp = get_undo_header(buf);

  ExtmarkUpdate update;
  update.ns_id = ns;
  update.mark_id = id;
  update.old_lnum = old_lnum;
  update.old_col = old_col;
  update.lnum = lnum;
  update.col = col;

  ExtmarkUndoObject undo = { .type = kExtmarkUpdate,
                             .reverse = extmarkNoReverse,
                             .data.update = update };
  kv_push(uhp->uh_extmark, undo);
}

// Hueristic works only for when the user is typing in insert mode
// Instead of 1 undo object for each chr, 1 for all text (before esc)
// Return True if we compacted else False
static bool u_compact_col_adjust(buf_T *buf,
                                 linenr_T lnum,
                                 colnr_T mincol,
                                 long lnum_amount,
                                 long col_amount,
                                 ExtmarkReverseType reverse) {
  if (reverse != extmarkNoReverse) {
    return false;
  }

  u_header_T  *uhp = get_undo_header(buf);
  if (kv_size(uhp->uh_extmark) < 1) {
    return false;
  }

  // Check the last action
  ExtmarkUndoObject object = kv_last(uhp->uh_extmark);
  ColAdjust undo = object.data.col_adjust;
  bool compactable = false;

  if (!undo.lnum_amount && !lnum_amount) {
    if (undo.lnum == lnum) {
      if ((undo.mincol + undo.col_amount) >= mincol) {
        if (object.reverse == extmarkNoReverse) {
          compactable = true;
  } } } }

  if (!compactable) {
    return false;
  }

  undo.col_amount = undo.col_amount + col_amount;
  ExtmarkUndoObject new_undo = { .type = kColAdjust,
                             .data.col_adjust = undo };
  kv_last(uhp->uh_extmark) = new_undo;
  return true;
}

// Save col_adjust info so we can undo/redo
static void u_extmark_col_adjust(buf_T *buf,
                                 linenr_T lnum,
                                 colnr_T mincol,
                                 long lnum_amount,
                                 long col_amount,
                                 ExtmarkReverseType reverse)
{
  u_header_T  *uhp = get_undo_header(buf);

  if (!u_compact_col_adjust(buf,
                            lnum, mincol, lnum_amount, col_amount, reverse)) {
    ColAdjust col_adjust;
    col_adjust.lnum = lnum;
    col_adjust.mincol = mincol;
    col_adjust.lnum_amount = lnum_amount;
    col_adjust.col_amount = col_amount;

    ExtmarkUndoObject undo = { .type = kColAdjust,
                               .reverse = reverse,
                               .data.col_adjust = col_adjust };

    kv_push(uhp->uh_extmark, undo);
  }
}

// Save adjust info so we can undo/redo
static void u_extmark_adjust(buf_T * buf,
                             linenr_T line1,
                             linenr_T line2,
                             long amount,
                             long amount_after,
                             ExtmarkReverseType reverse)
{
  u_header_T  *uhp = get_undo_header(buf);

  Adjust adjust;
  adjust.line1 = line1;
  adjust.line2 = line2;
  adjust.amount = amount;
  adjust.amount_after = amount_after;

  ExtmarkUndoObject undo = { .type = kAdjust,
                             .reverse = reverse,
                             .data.adjust = adjust };

  kv_push(uhp->uh_extmark, undo);
}

// save info to undo/redo a :move
void u_extmark_move(buf_T *buf,
                    linenr_T line1,
                    linenr_T line2,
                    linenr_T last_line,
                    linenr_T dest,
                    linenr_T num_lines,
                    linenr_T extra) {
  u_header_T  *uhp = get_undo_header(buf);

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

// helper to iterate over the information to undo/redo
// use the out parameters "from" and "to" if they are not -1
// otherwise use the return value
// param: int i, the state, where we are in the list
// param: undo, true if undo, false if redo
int extmark_iter_undo(extmark_undo_vec_t all_undos,
                     bool undo,
                     int i,
                     int *from,
                     int *to)
{
  *from = -1; *to = -1;
  ExtmarkUndoObject undo_info = kv_A(all_undos, i);

  if (undo_info.reverse == extmarkNoReverse) {
    return i;
  }

  if (undo_info.type == kAdjustMove) {
    return i;
  }

  // find the interval to reverse over
  for (; i > -1; i--) {
    undo_info = kv_A(all_undos, i);
    if (undo_info.reverse == extmarkReverseEnd) {
      *to = i;
      i--;
      break;
    }
  }
  for (; i > -1; i--) {
    undo_info = kv_A(all_undos, i);
    if (undo_info.reverse == extmarkNoReverse
        || undo_info.reverse == extmarkReverseEnd) {
      *from = i + 1;
      break;
    }
  }
  // edge case, if the last action is a reversal action
  if (*from == -1) {
    *from = 0;
  }  // finished finding the reverse interval

  return 1;  // Value not needed
}

void extmark_apply_undo(ExtmarkUndoObject undo_info, bool undo)
{
  linenr_T lnum;
  colnr_T mincol;
  long lnum_amount;
  long col_amount;
  linenr_T line1;
  linenr_T line2;
  long amount;
  long amount_after;

  // use extmark_column_adjust
  if (undo_info.type == kColAdjust) {
    // undo
    if (undo) {
      lnum = (undo_info.data.col_adjust.lnum
              + undo_info.data.col_adjust.lnum_amount);
      lnum_amount = -undo_info.data.col_adjust.lnum_amount;
      col_amount = -undo_info.data.col_adjust.col_amount;
      mincol = (undo_info.data.col_adjust.mincol
                + (colnr_T)undo_info.data.col_adjust.col_amount);
    // redo
    } else {
      lnum = undo_info.data.col_adjust.lnum;
      col_amount = undo_info.data.col_adjust.col_amount;
      lnum_amount = undo_info.data.col_adjust.lnum_amount;
      mincol = undo_info.data.col_adjust.mincol;
    }
    extmark_col_adjust(curbuf,
                       lnum, mincol, lnum_amount, col_amount, extmarkNoUndo);
  // use extmark_adjust
  } else if (undo_info.type == kAdjust) {
    if (undo) {
      // Undo - call signature type one - insert now
      if (undo_info.data.adjust.amount == MAXLNUM) {
        line1 = undo_info.data.adjust.line1;
        line2 = MAXLNUM;
        amount = -undo_info.data.adjust.amount_after;
        amount_after = 0;
      // Undo - call singature type two - delete now
      } else if (undo_info.data.adjust.line2 == MAXLNUM) {
        line1 = undo_info.data.adjust.line1;
        line2 = undo_info.data.adjust.line2;
        amount = -undo_info.data.adjust.amount;
        amount_after = undo_info.data.adjust.amount_after;
      // Undo - call signature three - move lines
      } else {
        line1 = (undo_info.data.adjust.line1
                 + undo_info.data.adjust.amount);
        line2 = (undo_info.data.adjust.line2
                 + undo_info.data.adjust.amount);
        amount = -undo_info.data.adjust.amount;
        amount_after = -undo_info.data.adjust.amount_after;
      }
    // redo
    } else {
      line1 = undo_info.data.adjust.line1;
      line2 = undo_info.data.adjust.line2;
      amount = undo_info.data.adjust.amount;
      amount_after = undo_info.data.adjust.amount_after;
    }
    extmark_adjust(curbuf,
                   line1, line2, amount, amount_after, extmarkNoUndo, false);
  // kAdjustMove
  } else if (undo_info.type == kAdjustMove) {
    apply_undo_move(undo_info, undo);
  // extmark_set
  } else if (undo_info.type == kExtmarkSet) {
    if (undo) {
      // TODO(timeyyy): Will curbuf alwasy be the correct buffer?
      extmark_unset(curbuf,
                    undo_info.data.set.ns_id,
                    undo_info.data.set.mark_id,
                    extmarkNoUndo);
    // Redo
    } else {
      extmark_set(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  undo_info.data.set.lnum,
                  undo_info.data.set.col,
                  extmarkNoUndo);
    }
  // extmark_set into update
  } else if (undo_info.type == kExtmarkUpdate) {
    if (undo) {
      extmark_set(curbuf,
                  undo_info.data.update.ns_id,
                  undo_info.data.update.mark_id,
                  undo_info.data.update.old_lnum,
                  undo_info.data.update.old_col,
                  extmarkNoUndo);
    // Redo
    } else {
      extmark_set(curbuf,
                  undo_info.data.update.ns_id,
                  undo_info.data.update.mark_id,
                  undo_info.data.update.lnum,
                  undo_info.data.update.col,
                  extmarkNoUndo);
    }
  // extmark_unset
  } else if (undo_info.type == kExtmarkUnset)  {
    if (undo) {
      extmark_set(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  undo_info.data.set.lnum,
                  undo_info.data.set.col,
                  extmarkNoUndo);
    // Redo
    } else {
      extmark_unset(curbuf,
                    undo_info.data.set.ns_id,
                    undo_info.data.set.mark_id,
                    extmarkNoUndo);
    }
  }
}

static void apply_undo_move(ExtmarkUndoObject undo_info, bool undo)
{
  // 3 calls are required , see comment in function do_move (ex_cmds.c)
  linenr_T line1 = undo_info.data.move.line1;
  linenr_T line2 = undo_info.data.move.line2;
  linenr_T last_line = undo_info.data.move.last_line;
  linenr_T dest = undo_info.data.move.dest;
  linenr_T num_lines = undo_info.data.move.num_lines;
  linenr_T extra = undo_info.data.move.extra;

  if (undo) {
    if (dest >= line2) {
      extmark_adjust(curbuf,
                     dest - num_lines + 1,
                     dest,
                     last_line - dest + num_lines - 1,
                     0L,
                     extmarkNoUndo,
                     true);
      extmark_adjust(curbuf,
                     dest - line2,
                     dest - line1,
                     dest - line2,
                     0L,
                     extmarkNoUndo,
                     false);
    } else {
      extmark_adjust(curbuf,
                     line1-num_lines,
                     line2-num_lines,
                     last_line - (line1-num_lines),
                     0L,
                     extmarkNoUndo,
                     true);
      extmark_adjust(curbuf,
                     (line1-num_lines) + 1,
                     (line2-num_lines) + 1,
                     -num_lines,
                     0L,
                     extmarkNoUndo,
                     false);
    }
    extmark_adjust(curbuf,
                   last_line,
                   last_line + num_lines - 1,
                   line1 - last_line,
                   0L,
                   extmarkNoUndo,
                   true);
  // redo
  } else {
    extmark_adjust(curbuf,
                   line1,
                   line2,
                   last_line - line2,
                   0L,
                   extmarkNoUndo,
                   true);
    if (dest >= line2) {
      extmark_adjust(curbuf,
                     line2 + 1,
                     dest,
                     -num_lines,
                     0L,
                     extmarkNoUndo,
                     false);
    } else {
      extmark_adjust(curbuf,
                     dest + 1,
                     line1 - 1,
                     num_lines,
                     0L,
                     extmarkNoUndo,
                     false);
    }
  extmark_adjust(curbuf,
                 last_line - num_lines + 1,
                 last_line,
                 -(last_line - dest - extra),
                 0L,
                 extmarkNoUndo,
                 true);
  }
}

// Adjust columns and rows for extmarks
// returns true if something was moved otherwise false
bool extmark_col_adjust(buf_T *buf, linenr_T lnum,
                        colnr_T mincol, long lnum_amount,
                        long col_amount, ExtmarkReverseType undo)
{
  linenr_T start;
  linenr_T end;
  if ((lnum + lnum_amount) < lnum) {
    start = lnum +lnum_amount;
    end = lnum;
  } else {
    start = lnum;
    end = lnum +lnum_amount;
  }

  bool marks_exist = false;
  linenr_T *lp;
  colnr_T *cp;

  FOR_ALL_EXTMARKS(buf, 0, start, 0, end, -1, {
    marks_exist = true;
    lp = &(extmark->line->lnum);
    cp = &(extmark->col);
    if (*lp == lnum && *cp >= mincol) {
      *lp += lnum_amount;
      assert(col_amount > INT_MIN && col_amount <= INT_MAX);
      // Delete mark
      if (col_amount < 0 && *cp <= (colnr_T)-col_amount) {
        extmark_unset(buf, extmark->ns_id, extmark->mark_id, extmarkNoReverse);
      } else {
        *cp += (colnr_T)col_amount;
      }
    }
  })

  if (undo != extmarkNoUndo
      && marks_exist) {
    u_extmark_col_adjust(buf, lnum, mincol, lnum_amount, col_amount, undo);
  }

  if (marks_exist) {
    return true;
  } else {
    return false;
  }
}

// Adjust extmark row for inserted/deleted rows (columns stay fixed).
void extmark_adjust(buf_T * buf,
                    linenr_T line1,
                    linenr_T line2,
                    long amount,
                    long amount_after,
                    ExtmarkReverseType undo,
                    bool end_temp)
{
  ExtMarkLine *_extline;

  // btree needs to be kept ordered to work, so far only :move requires this
  // 2nd call with end_temp =  unpack the lines from the temp position
  if (end_temp && amount < 0) {
    for (size_t i = 0; i < kv_size(buf->b_extmark_end_temp_space); i++) {
      _extline = kv_A(buf->b_extmark_end_temp_space, i);
      _extline->lnum += amount;
      kb_put(extlines, &buf->b_extlines, _extline);
    }
    kv_size(buf->b_extmark_end_temp_space) = 0;
    return;
  }

  bool marks_exist = false;
  linenr_T *lp;
  FOR_ALL_EXTMARKLINES(buf, 0, -1, {
    marks_exist = true;
    // _one_adjust_nodel(&(extline->lnum));
    lp = &(extline->lnum);
    if (*lp >= line1 && *lp <= line2) {
      // 1st call with end_temp = true, store the lines in a temp position
      if (end_temp && amount > 0) {
        kb_del_itr(extlines, &buf->b_extlines, &itr);
        kv_push(buf->b_extmark_end_temp_space, extline);
      }

      // Delete the line
      if (amount == MAXLNUM) {
        FOR_ALL_EXTMARKS_IN_LINE(&extline->items, {
          extmark_unset(buf, extmark->ns_id, extmark->mark_id,
                        extmarkNoReverse);
        })
      } else {
        *lp += amount;
      }
    } else if (amount_after && *lp > line2) {
      *lp += amount_after;
    }
  })

  if (undo != extmarkNoUndo
      && marks_exist) {
    u_extmark_adjust(buf, line1, line2, amount, amount_after, undo);
  }
}

/// Get reference to line in kbtree_t, allocating it if neccessary.
ExtMarkLine *extline_ref(kbtree_t(extlines) *b, linenr_T lnum)
{
  ExtMarkLine t, **pp;
  t.lnum = lnum;

  pp = kb_get(extlines, b, &t);
  if (!pp) {
    ExtMarkLine *p = xcalloc(sizeof(*p), 1);
    p->lnum = lnum;
    // p->items zero initialized
    kb_put(extlines, b, p);
    return p;
  }
  // Return existing
  return *pp;
}

void extmark_put(kbtree_t(markitems) *b,
                 colnr_T col,
                 uint64_t id,
                 ExtMarkLine *extline,
                 uint64_t ns)
{
  ExtendedMark t;
  t.col = col;
  t.mark_id = id;
  t.line = extline;
  t.ns_id = ns;

  kb_put(markitems, b, t);
}

// We only need to compare columns as rows are stored in different trees.
// Marks are ordered by: position, namespace, mark_id
// This improves moving marks but slows down all other use cases (searches)
int mark_cmp(ExtendedMark a, ExtendedMark b) {
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
