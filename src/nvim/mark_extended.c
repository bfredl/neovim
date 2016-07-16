/*
 * Same function but names are not limited to one char
 * There is also no ex_command, just viml functions
 */
#include <stdio.h>             // str

#include "nvim/vim.h"
#include "nvim/globals.h"      // FOR_ALL_BUFFERS
#include "nvim/mark.h"         // SET_FMARK
/* #include "nvim/memory.h" //TODO del? */
#include "nvim/map.h"          // pmap ...
#include "nvim/lib/kbtree.h"   // kbitr ...
#include "nvim/mark_extended.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.c.generated.h"
#endif

#define FOR_ALL_EXTMARKS(buf) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  kb_itr_first(str, buf->b_extmarks_tree, &itr);\
  for (; kb_itr_valid(&itr); kb_itr_next(cstr_t, buf->b_extmarks_tree, &itr)){\
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

/* Will fail silently on missing name */
int extmark_unset(buf_T *buf, char *name)
{
  ExtendedMark *extmark = get_extmark(buf, name);
  if (!extmark){
    return FAIL;
  }
  return extmark_delete(buf, name);
}

/* Given a text position, finds the next mark */
ExtendedMark *extmark_next(pos_T *pos, int fnum)
{
  ExtendedMark *extmark = find_pos(pos, 1, fnum);
  return extmark;
}

ExtendedMark *extmark_prev(pos_T *pos, int fnum)
{
  ExtendedMark *extmark = find_pos(pos, 0, fnum);
  return extmark;
}

/* Get all mark names */
/* kvec_t(cstr_t) extmark_names(char *name) */
/* { */
  /* kvec_t(cstr_t) array; */
  /* FOR_ALL_EXTMARKS{ */
    /* extmark = &kb_itr_key(ExtendedMark, &itr); */
    /* kv_push(array, extmark->name); */
  /* } */
  /* return array; */

static ExtendedMark *find_pos(pos_T *pos, bool go_forward, int fnum)
{
  int found = -1;
  if (go_forward == 1){
    found = 1;
  }
  int cmp;
  buf_T *buf = extmark_buf_from_fnum(fnum);
  FOR_ALL_EXTMARKS(buf)
    cmp = pos_cmp(*pos, extmark->fmark.mark);
    if (cmp == found) {
      return extmark->prev;
    }
    else if (cmp == 0) {
      return extmark;
    }
  END_LOOP
  return NULL;
}

static int extmark_create(buf_T *buf, char *name,  pos_T *pos)
{
  if (buf->b_extmarks == NULL) {
    buf->b_extmarks = pmap_new(cstr_t)();
    buf->b_extmarks_tree = kb_init(str, KB_DEFAULT_SIZE);
  }
  ExtendedMark extmark;
  /* fmark_T fmark; */
  /* extmark.fmark = fmark; */
  kb_put(ExtendedMark, buf->b_extmarks_tree,  extmark);
  pmap_put(cstr_t)(buf->b_extmarks, name, &extmark);
  SET_FMARK(&buf->b_extmarks, *pos, buf->b_fnum);
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
  pmap_del(ptr_t)(buf->b_extmarks, name);
  return OK;
}

// TODO use builtin
buf_T *extmark_buf_from_fnum(int fnum)
{
  buf_T *buf;
  FOR_ALL_BUFFERS(buf){
    if (fnum == buf->b_fnum){
        return buf;
    }
  }
  return buf;
}

/* returns an extmark from it's buffer*/
static ExtendedMark *get_extmark(buf_T *buf, char *name)
{
  return pmap_get(ExtendedMark)(buf->b_extmarks, name);
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
