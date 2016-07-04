/*
 * Same function but names are not limited to one char
 * There is also no ex_command, just viml functions
 */

#include "nvim/vim.h"
#include "nvim/globals.h"      // FOR_ALL_BUFFERS
#include "nvim/mark.h"         // SET_FMARK
#include "nvim/memory.h"
#include "nvim/map.h"          // pmap ...
#include "nvim/lib/kbtree.h"   // kbitr ...

#define FOR_ALL_ARBMARKS(buf) \
  kbitr_t = itr; \
  arbmark_T = *arbmark; \
  for (;kb_itr_valid(&itr) \
       ;kb_itr_next(arbmark_T, buf->b_arbmarks_tree, &itr)){

/* Create or update an arbmark, */
int arbmark_set(buf_T *buf, char_u *name, pos_T *pos)
{
  arbmark_T *arbmark = get_arbmark(buf, name);
  if (!arbmark){
    return arbmark_create(buf, name, pos);
  }
  else {
    return arbmark_update(arbmark, pos);
  }
}

/* Will fail silently on missing name */
int arbmark_unset(buf_T *buf, char_u *name)
{
  arbmark_T *arbmark = get_arbmark(buf, name);
  if (!arbmark){
    return FAIL;
  }
  return arbmark_delete(buf, name);
}

/* Given a text position, finds the next mark */
arbmark_T *arbmark_next(pos_T pos, int fnum)
{
  arbmark_T *arbmark = find_pos(pos, 1, fnum);
  return arbmark;
}

arbmark_T *arbmark_prev(pos_T pos, int fnum)
{
  arbmark_T *arbmark = find_pos(pos, 0, fnum);
  return arbmark;
}

/* Get all mark names */
/* kvec_t(cstr_t) arbmark_names(char_u *name) */
/* { */
  /* kvec_t(cstr_t) array; */
  /* FOR_ALL_ARBMARKS{ */
    /* arbmark = &kb_itr_key(arbmark_T, &itr); */
    /* kv_push(array, arbmark->name); */
  /* } */
  /* return array; */

static arbmark_T *find_pos(pos_T pos, bool go_forward, int fnum)
{
  int found = -1;
  if (go_forward == 1){
    found = 1;
  }
  buf_T *buf = arbmark_buf_from_fnum(fnum);
  FOR_ALL_ARBMARKS(buf){
    arbmark = &kb_itr_key(arbmark_T, &itr);
    int cmp = kb_generic_cmp(pos, arbmark->fmark.mark);
    switch (cmp){
      case !found:
        continue;
      case found:
        return arbmark.prev;
      case 0:
        return arbmark;
    }
  }
  return NULL;
}

static int arbmark_create(buf_T *buf, char_u *name,  pos_T *pos)
{
  arbmark_T arbmark;
  /* fmark_T fmark; */
  /* arbmark.fmark = fmark; */
  kb_putp(arbmark_T, buf->b_arbmarks_tree,  &arbmark);
  pmap_put(cstr_t)(buf->b_arbmarks, name, &arbmark);
  SET_FMARK(&buf->b_arbmarks, *pos, buf->b_fnum);
  return OK;
}

static int arbmark_update(arbmark_T *arbmark, pos_T *pos)
{
  arbmark->mark.lnum = pos->lnum;
  arbmark->mark.col = pos->col;
  return OK;
}

static int arbmark_delete(buf_T *buf, char_u *name)
{
  pmap_del(ptr_t)(buf->b_arbmarks, name);
  return OK;
}

// TODO start with current buffer
// TODO use a lru cache,
buf_T *arbmark_buf_from_fnum(int fnum)
{
  buf_T *buf;
  FOR_ALL_BUFFERS(buf){
    if (fnum == buf->fnum){
        return buf;
    }
  }
  return buf;
}

/* returns an arbmark from it's buffer*/
static arbmark_T *get_arbmark(buf_T *buf, char_u *name)
{
  return pmap_get(arbmark_T)(buf->b_arbmarks, name);
}

int _pos_cmp(pos_T a, pos_T b)
{
  if (pos_lt(a, b) == OK){
    return -1;
  }
  else if (pos_eq(a, b) == OK){
    return 0;
  }
  else {
    return 1;
  }
}

static int pos_lt(pos_T *pos, pos_T *pos2){
  if (pos->lnum < pos2->lnum) {
      if (pos->col < pos2->col) {
          return OK;
      }
  }
  return FAIL;
}

static int pos_eq(pos_T *pos, pos_T *pos2)
{
  if (pos->lnum == pos2->lnum) {
      if (pos->col == pos2->col) {
          return OK;
      }
  }
  return FAIL;
}
