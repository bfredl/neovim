
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
// is applied in extmark_apply_undo
//
// Marks live in namespaces that allow plugins/users to segregate marks
// from other users, namespaces have to be initialized before usage
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

uint64_t extmark_namespace_counter = 1;

// Required before calling any other functions
uint64_t extmark_ns_create(char *ns)
{
  if (!EXTMARK_NAMESPACES) {
    EXTMARK_NAMESPACES = map_new(uint64_t, cstr_t)();
  }
  uint64_t id = extmark_namespace_counter++;
  map_put(uint64_t, cstr_t)(EXTMARK_NAMESPACES, id, xstrdup(ns));
  return id;
}

bool ns_initialized(uint64_t ns)
{
  return ns < extmark_namespace_counter;
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
                ExtmarkOp op)
{
  ExtendedMark *extmark = extmark_from_id(buf, ns, id);
  if (!extmark) {
    return extmark_create(buf, ns, id, lnum, col, op);
  } else {
    extmark_update(extmark, buf, ns, id, lnum,  col, op, NULL);
    return 2;
  }
}

// Returns 0 on missing id
int extmark_unset(buf_T *buf,
                  uint64_t ns,
                  uint64_t id,
                  ExtmarkOp op)
{
  ExtendedMark *extmark = extmark_from_id(buf, ns, id);
  if (!extmark) {
    return 0;
  }
  return extmark_delete(extmark, buf, ns, id, op);
}

// Returns the position of marks between a range,
// marks found at the start or end index will be included,
// if upper_lnum or upper_col are negative the buffer
// will be searched to the start, or end
// dir can be set to control the order of the array
// amount = amount of marks to find or -1 for all
ExtmarkArray extmark_get(buf_T *buf,
                         uint64_t ns,
                         linenr_T l_lnum,
                         colnr_T l_col,
                         linenr_T u_lnum,
                         colnr_T u_col,
                         int64_t amount,
                         int dir)
{
  ExtmarkArray array = KV_INITIAL_VALUE;
  // Find all the marks
  if (amount == -1) {
    if (dir == FORWARD) {
      FOR_ALL_EXTMARKS(buf, ns, l_lnum, l_col, u_lnum, u_col, {
        if (extmark->ns_id == ns) {
          kv_push(array, extmark);
        }
      })
    } else {
      FOR_ALL_EXTMARKS_PREV(buf, ns, l_lnum, l_col, u_lnum, u_col, {
        if (extmark->ns_id == ns) {
          kv_push(array, extmark);
        }
      })
    }
  // Find only a certain amount of marks (only thing extra is the size check )
  } else {
    if (dir == FORWARD) {
      FOR_ALL_EXTMARKS(buf, ns, l_lnum, l_col, u_lnum, u_col, {
        if (extmark->ns_id == ns) {
          kv_push(array, extmark);
          if (kv_size(array) == (size_t)amount) {
            return array;
          }
        }
      })
    } else {
      FOR_ALL_EXTMARKS_PREV(buf, ns, l_lnum, l_col, u_lnum, u_col, {
        if (extmark->ns_id == ns) {
          kv_push(array, extmark);
          if (kv_size(array) == (size_t)amount) {
            return array;
          }
        }
      })
    }
  }
  return array;
}

static bool extmark_create(buf_T *buf,
                           uint64_t ns,
                           uint64_t id,
                           linenr_T lnum,
                           colnr_T col,
                           ExtmarkOp op)
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

  // Create or get a line
  ExtMarkLine *extline = extline_ref(&buf->b_extlines, lnum);
  // Create and put mark on the line
  extmark_put(col, id, extline, ns);

  // Marks do not have stable address so we have to look them up
  // by using the line instead of the mark
  pmap_put(uint64_t)(ns_obj->map, id, extline);
  if (op != kExtmarkNoUndo) {
    u_extmark_set(buf, ns, id, lnum, col, kExtmarkSet);
  }

  // Set a free id so extmark_free_id_get works
  extmark_free_id_set(ns_obj, id);
  return true;
}

// update the position of an extmark
// to update while iterating pass the markitems itr
static void extmark_update(ExtendedMark *extmark,
                           buf_T *buf,
                           uint64_t ns,
                           uint64_t id,
                           linenr_T lnum,
                           colnr_T col,
                           ExtmarkOp op,
                           kbitr_t(markitems) *mitr)
{
  assert(op != kExtmarkNOOP);
  if (op != kExtmarkNoUndo) {
    u_extmark_update(buf, ns, id, extmark->line->lnum, extmark->col,
                     lnum, col);
  }
  ExtMarkLine *old_line = extmark->line;
  // Move the mark to a new line and update column
  if (old_line->lnum != lnum) {
    ExtMarkLine *ref_line = extline_ref(&buf->b_extlines, lnum);
    extmark_put(col, id, ref_line, ns);
    // Update the hashmap
    ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
    pmap_put(uint64_t)(ns_obj->map, id, ref_line);
    // Delete old mark
    if (mitr != NULL) {
      kb_del_itr(markitems, &(old_line->items), mitr);
    } else {
      kb_del(markitems, &(old_line->items), *extmark);
    }
  // Just update the column
  } else {
    extmark->col = col;
  }

}

static int extmark_delete(ExtendedMark *extmark,
                          buf_T *buf,
                          uint64_t ns,
                          uint64_t id,
                          ExtmarkOp op)
{
  if (op != kExtmarkNoUndo) {
    u_extmark_set(buf, ns, id, extmark->line->lnum, extmark->col,
                  kExtmarkUnset);
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

  FOR_ALL_EXTMARKS_IN_LINE(extline->items, {
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

// Returns an avaliable id in a namespace
uint64_t extmark_free_id_get(buf_T *buf, uint64_t ns)
{
  uint64_t free_id = 0;

  if (!buf->b_extmark_ns) {
    return free_id;
  }
  ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
  if (!ns_obj) {
    return free_id;
  }
  return ns_obj->free_id;
}

// Set the free id in a namesapce
static void extmark_free_id_set(ExtmarkNs *ns_obj, uint64_t id)
{
  // Simply Heurstic, the largest id + 1
  ns_obj->free_id = id + 1;
}

void extmark_free_all(buf_T *buf)
{
  if (!buf->b_extmark_ns) {
    return;
  }

  uint64_t ns;
  ExtmarkNs *ns_obj;

  map_foreach(buf->b_extmark_ns, ns, ns_obj, {
     FOR_ALL_EXTMARKS(buf, ns, MINLNUM, MINCOL, MAXLNUM, MAXCOL, {
       kb_del_itr(markitems, &extline->items, &mitr);
     });
    pmap_free(uint64_t)(ns_obj->map);
    xfree(ns_obj);
  });

  pmap_free(uint64_t)(buf->b_extmark_ns);

  FOR_ALL_EXTMARKLINES(buf, MINLNUM, MAXLNUM, {
    kb_destroy(markitems, (&extline->items));
    kb_del_itr(extlines, &buf->b_extlines, &itr);
    xfree(extline);
  })
  // k?_init called to set pointers to NULL
  kb_destroy(extlines, (&buf->b_extlines));
  kb_init(&buf->b_extlines);

  kv_destroy(buf->b_extmark_move_space);
  kv_init(buf->b_extmark_move_space);
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
                             .op = kExtmarkUndo,
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
                             .op = kExtmarkUndo,
                             .data.update = update };
  kv_push(uhp->uh_extmark, undo);
}

// Hueristic works only for when the user is typing in insert mode
// - Instead of 1 undo object for each char inserted,
//   we create 1 undo objet for all text inserted before the user hits esc
// Return True if we compacted else False
static bool u_compact_col_adjust(buf_T *buf,
                                 linenr_T lnum,
                                 colnr_T mincol,
                                 long lnum_amount,
                                 long col_amount,
                                 ExtmarkOp op)
{
  if (op != kExtmarkUndo) {
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
        if (object.op == kExtmarkUndo) {
          compactable = true;
  } } } }

  if (!compactable) {
    return false;
  }

  undo.col_amount = undo.col_amount + col_amount;
  ExtmarkUndoObject new_undo = { .type = kColAdjust,
                                 .op = kExtmarkUndo,
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
                                 ExtmarkOp op)
{
  u_header_T  *uhp = get_undo_header(buf);

  if (!u_compact_col_adjust(buf,
                            lnum, mincol, lnum_amount, col_amount, op)) {
    ColAdjust col_adjust;
    col_adjust.lnum = lnum;
    col_adjust.mincol = mincol;
    col_adjust.lnum_amount = lnum_amount;
    col_adjust.col_amount = col_amount;

    ExtmarkUndoObject undo = { .type = kColAdjust,
                               .op = op,
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
                             ExtmarkOp op)
{
  u_header_T  *uhp = get_undo_header(buf);

  Adjust adjust;
  adjust.line1 = line1;
  adjust.line2 = line2;
  adjust.amount = amount;
  adjust.amount_after = amount_after;

  ExtmarkUndoObject undo = { .type = kAdjust,
                             .op = op,
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
                    linenr_T extra)
{
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
                       lnum, mincol, lnum_amount, col_amount, kExtmarkNoUndo);
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
                   line1, line2, amount, amount_after, kExtmarkNoUndo, false);
  // kAdjustMove
  } else if (undo_info.type == kAdjustMove) {
    apply_undo_move(undo_info, undo);
  // extmark_set
  } else if (undo_info.type == kExtmarkSet) {
    if (undo) {
      extmark_unset(curbuf,
                    undo_info.data.set.ns_id,
                    undo_info.data.set.mark_id,
                    kExtmarkNoUndo);
    // Redo
    } else {
      extmark_set(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  undo_info.data.set.lnum,
                  undo_info.data.set.col,
                  kExtmarkNoUndo);
    }
  // extmark_set into update
  } else if (undo_info.type == kExtmarkUpdate) {
    if (undo) {
      extmark_set(curbuf,
                  undo_info.data.update.ns_id,
                  undo_info.data.update.mark_id,
                  undo_info.data.update.old_lnum,
                  undo_info.data.update.old_col,
                  kExtmarkNoUndo);
    // Redo
    } else {
      extmark_set(curbuf,
                  undo_info.data.update.ns_id,
                  undo_info.data.update.mark_id,
                  undo_info.data.update.lnum,
                  undo_info.data.update.col,
                  kExtmarkNoUndo);
    }
  // extmark_unset
  } else if (undo_info.type == kExtmarkUnset)  {
    if (undo) {
      extmark_set(curbuf,
                  undo_info.data.set.ns_id,
                  undo_info.data.set.mark_id,
                  undo_info.data.set.lnum,
                  undo_info.data.set.col,
                  kExtmarkNoUndo);
    // Redo
    } else {
      extmark_unset(curbuf,
                    undo_info.data.set.ns_id,
                    undo_info.data.set.mark_id,
                    kExtmarkNoUndo);
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
                     kExtmarkNoUndo,
                     true);
      extmark_adjust(curbuf,
                     dest - line2,
                     dest - line1,
                     dest - line2,
                     0L,
                     kExtmarkNoUndo,
                     false);
    } else {
      extmark_adjust(curbuf,
                     line1-num_lines,
                     line2-num_lines,
                     last_line - (line1-num_lines),
                     0L,
                     kExtmarkNoUndo,
                     true);
      extmark_adjust(curbuf,
                     (line1-num_lines) + 1,
                     (line2-num_lines) + 1,
                     -num_lines,
                     0L,
                     kExtmarkNoUndo,
                     false);
    }
    extmark_adjust(curbuf,
                   last_line,
                   last_line + num_lines - 1,
                   line1 - last_line,
                   0L,
                   kExtmarkNoUndo,
                   true);
  // redo
  } else {
    extmark_adjust(curbuf,
                   line1,
                   line2,
                   last_line - line2,
                   0L,
                   kExtmarkNoUndo,
                   true);
    if (dest >= line2) {
      extmark_adjust(curbuf,
                     line2 + 1,
                     dest,
                     -num_lines,
                     0L,
                     kExtmarkNoUndo,
                     false);
    } else {
      extmark_adjust(curbuf,
                     dest + 1,
                     line1 - 1,
                     num_lines,
                     0L,
                     kExtmarkNoUndo,
                     false);
    }
  extmark_adjust(curbuf,
                 last_line - num_lines + 1,
                 last_line,
                 -(last_line - dest - extra),
                 0L,
                 kExtmarkNoUndo,
                 true);
  }
}

// Adjust columns and rows for extmarks
// based off mark_col_adjust in mark.c
// returns true if something was moved otherwise false
bool extmark_col_adjust(buf_T *buf, linenr_T lnum,
                        colnr_T mincol, long lnum_amount,
                        long col_amount, ExtmarkOp undo)
{
  assert(col_amount > INT_MIN && col_amount <= INT_MAX);

  bool marks_exist = false;
  colnr_T *cp;

  FOR_ALL_EXTMARKLINES(buf, lnum, lnum, {
    FOR_ALL_EXTMARKS_IN_LINE(extline->items, {
      marks_exist = true;
      cp = &(extmark->col);
      // Delete mark
      if (col_amount < 0
          && *cp <= (colnr_T)-col_amount
          && *cp > mincol) {  // TODO(timeyyy): does mark.c need this line?
        extmark_unset(buf, extmark->ns_id, extmark->mark_id,
                      kExtmarkUndo);
      // Update the mark
      } else if (*cp >= mincol) {
        // Note: The undo is handled by u_extmark_col_adjust, NoUndo here
        extmark_update(extmark, buf, extmark->ns_id, extmark->mark_id,
                       extline->lnum + lnum_amount,
                       *cp + (colnr_T)col_amount, kExtmarkNoUndo, &mitr);
      }
    })
  })

  if (undo != kExtmarkNoUndo
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
                    ExtmarkOp undo,
                    bool end_temp)
{
  ExtMarkLine *_extline;

  // btree needs to be kept ordered to work, so far only :move requires this
  // 2nd call with end_temp =  unpack the lines from the temp position
  if (end_temp && amount < 0) {
    for (size_t i = 0; i < kv_size(buf->b_extmark_move_space); i++) {
      _extline = kv_A(buf->b_extmark_move_space, i);
      _extline->lnum += amount;
      kb_put(extlines, &buf->b_extlines, _extline);
    }
    kv_size(buf->b_extmark_move_space) = 0;
    return;
  }

  bool marks_exist = false;
  linenr_T *lp;
  FOR_ALL_EXTMARKLINES(buf, MINLNUM, MAXLNUM, {
    marks_exist = true;
    lp = &(extline->lnum);
    if (*lp >= line1 && *lp <= line2) {
      // 1st call with end_temp = true, store the lines in a temp position
      if (end_temp && amount > 0) {
        kb_del_itr(extlines, &buf->b_extlines, &itr);
        kv_push(buf->b_extmark_move_space, extline);
      }

      // Delete the line
      if (amount == MAXLNUM) {
        FOR_ALL_EXTMARKS_IN_LINE(extline->items, {
          extmark_unset(buf, extmark->ns_id, extmark->mark_id,
                        kExtmarkUndo);
        })
        // TODO(timeyyy): make freeing the line a undoable action
        // Maybe a pool of lines...
        // kb_del_itr(extlines, &buf->b_extlines, &itr);
        // xfree(extline);
      } else {
        *lp += amount;
      }
    } else if (amount_after && *lp > line2) {
      *lp += amount_after;
    }
  })

  if (undo != kExtmarkNoUndo
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
    ExtMarkLine *p = xcalloc(sizeof(ExtMarkLine), 1);
    p->lnum = lnum;
    // p->items zero initialized
    kb_put(extlines, b, p);
    return p;
  }
  // Return existing
  return *pp;
}

// Put an extmark into a line, combination of id and ns_id must be unique
void extmark_put(colnr_T col,
                 uint64_t id,
                 ExtMarkLine *extline,
                 uint64_t ns)
{
  ExtendedMark t;
  t.col = col;
  t.mark_id = id;
  t.line = extline;
  t.ns_id = ns;

  kbtree_t(markitems) *b = &(extline->items);
  // kb_put requries the key to not be there
  assert(!kb_getp(markitems, b, &t));

  kb_put(markitems, b, t);
}

// We only need to compare columns as rows are stored in different trees.
// Marks are ordered by: position, namespace, mark_id
// This improves moving marks but slows down all other use cases (searches)
int mark_cmp(ExtendedMark a, ExtendedMark b)
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
