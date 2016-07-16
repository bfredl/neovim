/*
 * Same function but names are not limited to one char
 * There is also no ex_command, just viml functions
 * All //TODO
 */

#include "nvim/vim.h"
#include "nvim/mark_extended.h"
#include "nvim/globals.h"      // FOR_ALL_BUFFERS
#include "nvim/mark.h"         // SET_FMARK
/* #include "nvim/memory.h" //TODO del? */
#include "nvim/map.h"          // pmap ...
#include "nvim/lib/kbtree.h"   // kbitr ...

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.c.generated.h"
#endif

#define FOR_ALL_EXTMARKS(buf) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  kb_itr_first(extmarks, buf->b_extmarks_tree, &itr);\
  for (; kb_itr_valid(&itr); kb_itr_next(extmarks, buf->b_extmarks_tree, &itr)){\
    extmark = &kb_itr_key(ExtendedMark, &itr);

#define END_LOOP }

/* Create or update an extmark, */
int extmark_set(buf_T *buf, char *name, pos_T *pos)
{
  ExtendedMark *extmark = get_extmark(buf, name);
  if (!extmark){
    return extmark_create(buf, name, pos);
  }
  else {
    return extmark_update(extmark, pos);
  }
}

/* returns FAIL on missing name */
int extmark_unset(buf_T *buf, char *name)
{
  ExtendedMark *extmark = get_extmark(buf, name);
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

/* Returns the postion of the given index  */
pos_T *extmark_index(buf_T *buf, char *name) {
  ExtendedMark *extmark = get_extmark(buf, name);
  return &extmark->fmark.mark;
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

static int extmark_create(buf_T *buf, char *name,  pos_T *pos)
{
  if (buf->b_extmarks == NULL) {
    buf->b_extmarks = pmap_new(cstr_t)();
    buf->b_extmarks_tree = kb_init(extmarks, KB_DEFAULT_SIZE);
  }
  ExtendedMark extmark;
  kb_put(extmarks, buf->b_extmarks_tree,  extmark);
  pmap_put(cstr_t)(buf->b_extmarks, name, &extmark);
  SET_FMARK(&extmark.fmark, *pos, buf->b_fnum);
  return OK;
}

static int extmark_update(ExtendedMark *extmark, pos_T *pos)
{
  extmark->fmark.mark.lnum = pos->lnum;
  extmark->fmark.mark.col = pos->col;
  return OK;
}

static int extmark_delete(buf_T *buf, char *name)
{
  pmap_del(cstr_t)(buf->b_extmarks, name);
  return OK;
}

// TODO use builtin
buf_T *extmark_buf_from_fnum(int fnum)
{
  buf_T *buf = NULL;
  FOR_ALL_BUFFERS(buf){
    if (fnum == buf->b_fnum){
        return buf;
    }
  }
  return buf;
}

static ExtendedMark *get_extmark(buf_T *buf, char *name)
{
  return pmap_get(cstr_t)(buf->b_extmarks, name);
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
