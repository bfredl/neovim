/*
 * A normal mark that you would find in other text editors
 * The marks exists seperatley to vim marks, there are no
 * special marks for insert cursort etc
 *
 * Marks are stored in a btree for fast searching
 * A map of pointers to the marks is used for fast lookup
 *
 * TODO Mark gravity to left or right.
 */

#include "nvim/vim.h"
#include "nvim/mark_extended.h"
#include "nvim/globals.h"      // FOR_ALL_BUFFERS
#include "nvim/mark.h"         // SET_FMARK
#include "nvim/map.h"          // pmap ...
#include "nvim/lib/kbtree.h"   // kbitr ...

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.c.generated.h"
#endif

/* Create or update an extmark, */
int extmark_set(buf_T *buf, char *name, linenr_T row, colnr_T col)
{
  ExtendedMark *extmark = extmark_get(buf, name);
  if (!extmark){
    return extmark_create(buf, name, row, col);
  }
  else {
    extmark_update(extmark, row,  col);
    return 2;
  }
}

/* returns FAIL on missing name */
int extmark_unset(buf_T *buf, char *name)
{
  ExtendedMark *extmark = extmark_get(buf, name);
  if (!extmark){
    return FAIL;
  }
  return extmark_delete(buf, name);
}

/* Get all mark names ordered by position*/
ExtmarkNames *extmark_names(buf_T *buf)
{
  ExtmarkNames extmark_names = KV_INITIAL_VALUE;
  ExtmarkNames *array = &extmark_names;
  FOR_ALL_EXTMARKS(buf)
    kv_push(extmark_names, extmark->name);
  END_LOOP
  return array;
}

/* Returns the postion of the given mark  */
pos_T *extmark_index(buf_T *buf, char *name) {
  ExtendedMark *extmark = extmark_get(buf, name);
  if (!extmark){
    return NULL;
  }
  return &(extmark->fmark.mark);
}

/* Given a text position, finds the next mark */
pos_T *extmark_next(buf_T *buf, pos_T *pos)
{
  return get_pos(buf, pos, 1);
}

/* Given a text position, finds the previous mark */
pos_T *extmark_prev(buf_T *buf, pos_T *pos)
{
  return get_pos(buf, pos, 0);
}

static pos_T *get_pos(buf_T *buf, pos_T *pos, bool go_forward)
{
  int found = -1;
  if (go_forward == 1){
    found = 1;
  }
  int cmp;
  FOR_ALL_EXTMARKS(buf)
    cmp = pos_cmp(*pos, extmark->fmark.mark);
    if (cmp == found) {
      return &extmark->prev->fmark.mark;
    }
    else if (cmp == 0) {
      return &extmark->fmark.mark;
    }
  END_LOOP
  return NULL;
}

static bool extmark_create(buf_T *buf, char *name, int row, int col)
{
  if (buf->b_extmarks == NULL) {
    buf->b_extmarks = pmap_new(cstr_t)();
    buf->b_extmarks_tree = kb_init(extmarks, KB_DEFAULT_SIZE);
  }
  // TODO set prev pointer
  ExtendedMark *extmark = (ExtendedMark *) malloc(sizeof(ExtendedMark));

  extmark->fmark.mark.lnum = row;
  extmark->fmark.mark.col = col;
  kb_put(extmarks, buf->b_extmarks_tree,  *extmark);
  pmap_put(cstr_t)(buf->b_extmarks, (cstr_t)name, extmark);
  // TODO do we need the timestamp and additional_data ??, also pos_t has 3 fields
  SET_FMARK(&extmark->fmark, extmark->fmark.mark, buf->b_fnum);

  return OK;
}

static void extmark_update(ExtendedMark *extmark, linenr_T row, colnr_T col)
{
  // TODO set prev pointer
  extmark->fmark.mark.lnum = row;
  extmark->fmark.mark.col = col;
}

static int extmark_delete(buf_T *buf, char *name)
{
  // TODO set prev pointer, remove from btree
  pmap_del(cstr_t)(buf->b_extmarks, name);
  return OK;
}

ExtendedMark *extmark_get(buf_T *buf, char *name)
{
  if (buf->b_extmarks == NULL) {
    return NULL;
  }
  return pmap_get(cstr_t)(buf->b_extmarks, (cstr_t)name);
}

int pos_cmp(pos_T a, pos_T b)
{
  if (a.lnum < b.lnum) {
    return -1;
  }
  else if (a.lnum == b.lnum) {
    if (a.col < b.col) {
      return -1;
    }
    else if (a.col == b.col) {
      return 0;
    }
  }
  return 1;
}

void extmark_free_all(buf_T *buf)
{
  if (!buf->b_extmarks) {
    return;
  }
  pmap_free(cstr_t)(buf->b_extmarks);
  kb_destroy(extmarks, buf->b_extmarks_tree);
}

//TODO  use from mark.c
#define _col_adjust(pp) \
  { \
    posp = pp; \
    if (posp->lnum == lnum && posp->col >= mincol) \
    { \
      posp->lnum += lnum_amount; \
      assert(col_amount > INT_MIN && col_amount <= INT_MAX); \
      if (col_amount < 0 && posp->col <= (colnr_T)-col_amount) \
        posp->col = 0; \
      else \
        posp->col += (colnr_T)col_amount; \
    } \
  }

/* Adjust exmarks when changes to columns happen */
/* This is called from mark_col_adjust as well as */
/* from mark_adjust, and from wherever text edits happen */
void extmark_col_adjust(buf_T *buf, linenr_T lnum, colnr_T mincol, long lnum_amount, long col_amount)
{
  pos_T *posp;
  FOR_ALL_EXTMARKS(buf)
    _col_adjust(&extmark->fmark.mark)
  END_LOOP
}

 /* Adjust extmark row for inserted/deleted rows. */
void extmark_adjust(buf_T* buf, linenr_T line1, linenr_T line2, long amount, long amount_after)
{
  FOR_ALL_EXTMARKS(buf)
    if (extmark->fmark.mark.lnum >= line1
        && extmark->fmark.mark.lnum <= line2) {
          if (amount == MAXLNUM) {
            extmark->fmark.mark.lnum = line1;
          }
          else {
            extmark->fmark.mark.lnum += amount;
          }
    }
    else if (extmark->fmark.mark.lnum > line2)
        extmark->fmark.mark.lnum += amount_after;
  END_LOOP
}

