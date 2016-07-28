/* A normal mark that you would find in other text editors
 * The marks exists seperatley to vim marks, there are no
 * special marks for insert cursort etc
 *
 * Marks are stored in a btree for fast searching
 * A map of pointers to the marks is used for fast lookup
 *
 * For moving marks see : http://blog.atom.io/2015/06/16/optimizing-an-important-atom-primitive.html
 *
 */

#include "nvim/vim.h"
#include "nvim/mark_extended.h"
#include "nvim/memory.h"
#include "nvim/globals.h"      // FOR_ALL_BUFFERS
/* #include "nvim/mark.h"         // SET_FMARK */
#include "nvim/map.h"          // pmap ...
#include "nvim/lib/kbtree.h"   // kbitr ...

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.c.generated.h"
#endif

/* bool extmark_create_namespace(buf_T *buf, char *ns */
/* { */
/* } */

/* Create or update an extmark */
/* returns two on succesful update */
/* returns three if namespace isn't found */ //TODO
int called=0;
int extmark_set(buf_T *buf, char *ns, char *name, linenr_T row, colnr_T col)
{
  called ++;
  if (called==2)
    return 0;
  ExtendedMark *extmark = extmark_get(buf, ns, name);
  if (!extmark){
    return extmark_create(buf, ns, name, row, col);
  }
  else {
    extmark_update(extmark, buf, ns, name, row,  col);
    return 2;
  }
}

/* returns 0 on missing name */
int extmark_unset(buf_T *buf, char *ns, char *name)
{
  ExtendedMark *extmark = extmark_get(buf, ns, name);
  if (!extmark){
    return FAIL;
  }
  return extmark_delete(extmark, buf, ns, name);
}

/* Get all mark names ordered by position*/
ExtmarkNames *extmark_names(buf_T *buf, char *ns)
{
  ExtmarkNames *array = (ExtmarkNames *)xmalloc(sizeof(ExtmarkNames));
  kv_init(*array);
  FOR_EXTMARKS_IN_NS(buf, ns)
    kv_push(*array, (char *)extmark->name);
  END_FOR_EXTMARKS_IN_NS
  return array;
}

/* Returns the postion of the given mark  */
pos_T *extmark_index(buf_T *buf, char *ns, char *name) {
  ExtendedMark *extmark = extmark_get(buf, ns, name);
  if (!extmark){
    return NULL;
  }
  return &(extmark->mark);
}

/* Given a mark, finds the next mark */
ExtendedMark *extmark_next(buf_T *buf, char *ns, pos_T *pos)
{
  return extmark_neighbour(buf, ns, pos, 1);
}

/* Given a name finds the previous mark */
ExtendedMark *extmark_prev(buf_T *buf, char *ns, pos_T *pos)
{
  return extmark_neighbour(buf, ns, pos, 0);
}

ExtmarkArray *extmark_nextrange(buf_T *buf, char *ns, pos_T *lower, pos_T *upper)
{
  return extmark_neighbour_range(buf, ns, lower, upper, 1);
}

/* Returns the position of marks between a range, */
/* marks found at the start or end index will be included, */
/* if upper.lnum or upper.col are negative the buffer */
/* will be searched to the end, */
/* go_forward can be set to control the order of the array */
static ExtmarkArray *extmark_neighbour_range(buf_T *buf, char *ns, pos_T *lower, pos_T *upper, bool go_forward)
{
  ExtmarkArray *array = (ExtmarkArray *)xmalloc(sizeof(ExtmarkArray));
  kv_init(*array);
  int cmp;

  FOR_EXTMARKS_IN_NS(buf, ns)
    if (pos_cmp(*lower, *upper) == 1) {
      break;
    }
    cmp = pos_cmp(*lower, extmark->mark);
    if (cmp != 1) {
      kv_push(*array, extmark);
    }
    else if (cmp == 1) {
      break;
    }
  END_FOR_EXTMARKS_IN_NS

  /* Swap the elements in the array */
  /* if (!go_forward) { */
      /* ExtendedMark holder; */
      /* int n = (int)sizeof(array); */
      /* int end = n-1; */
      /* for (int i = 0; i < n/2; i++) { */
          /* holder = kv_A(*array, i); */
          /* array[i] = array[end]; */
          /* array[end] = holder; */
          /* end --; */
      /* } */
  /* } */
  return array;
}

/* Returns the mark, nearest to the passed in mark*/
static ExtendedMark *extmark_neighbour(buf_T *buf, char *ns, pos_T *input, bool go_forward)
{
  int cmp;
  if (go_forward) {
    FOR_ALL_EXTMARKS(buf)
      cmp = pos_cmp(extmark->mark, *input);
      if (cmp == 1) {
        return extmark;
      }
    END_FOR_ALL_EXTMARKS
  } else {
    FOR_ALL_EXTMARKS_WITH_PREV(buf)
      cmp = pos_cmp(extmark->mark, *input);
      if (cmp == 1) {
        return prev;
      }
    END_FOR_ALL_EXTMARKS_WITH_PREV
  }
  return NULL;
}
static bool extmark_create(buf_T *buf, char* ns, char *name, linenr_T row, colnr_T col)
{
  /* Initialize as first mark on this buffer*/
  if (!buf->b_extmarks) {
    buf->b_extmarks = kb_init(extmarks, KB_DEFAULT_SIZE);
    buf->b_extmark_ns = pmap_new(cstr_t)();
  }
  ExtmarkNamespace *ns_obj = pmap_get(cstr_t)(buf->b_extmark_ns, (cstr_t)ns);
  /* Initialize a new namespace */
  /* if (!ns_map || !kh_size(ns_map->table)) { */
  ns = xstrdup(ns);
  if (!ns_obj) {
    ns_obj = (ExtmarkNamespace *)xmalloc(sizeof(ExtmarkNamespace));
    ns_obj->map = pmap_new(cstr_t)();
    ns_obj->tree = kb_init(extmarks, KB_DEFAULT_SIZE);
    pmap_put(cstr_t)(buf->b_extmark_ns, (cstr_t)ns, ns_obj);
  }

  name = xstrdup(name);
  ExtendedMark *extmark = (ExtendedMark *)xmalloc(sizeof(ExtendedMark));
  /* extmark->mark = gen_relative(buf, ns, row, col); */
  extmark->mark.lnum = row;
  extmark->mark.col = col;
  extmark->name = name;

  kb_put(extmarks, buf->b_extmarks, extmark);
  kb_put(extmarks, ns_obj->tree, extmark);

  pmap_put(cstr_t)(ns_obj->map, (cstr_t)name, extmark);

  return OK;
}

static void extmark_update(ExtendedMark *extmark, buf_T *buf, char *ns, char *name, linenr_T row, colnr_T col)
{
  extmark->mark.lnum = row;
  extmark->mark.col = col;
  /* gen_relative(); */
}

static int extmark_delete(ExtendedMark *extmark, buf_T *buf, char *ns, char *name)
{
  /* Remove our key from the namespace */
  ExtmarkNamespace *ns_obj = pmap_get(cstr_t)(buf->b_extmark_ns, (cstr_t)ns);
  pmap_del(cstr_t)(ns_obj->map, (cstr_t)name);

  /* Delete the mark */
  kb_del(extmarks, buf->b_extmarks, extmark);
  kb_del(extmarks, ns_obj->tree, extmark);
  xfree(extmark); // wasn't required before change to storing pointers

  return OK;
}

ExtendedMark *extmark_get(buf_T *buf, char *ns, char *name)
{
  if (!buf->b_extmarks) {
    return NULL;
  }
  ExtmarkNamespace *ns_obj = pmap_get(cstr_t)(buf->b_extmark_ns, (cstr_t)ns);
  if (!ns_obj) { // || !kh_size(ns_map->table)) {
    return NULL;
  }
  return pmap_get(cstr_t)(ns_obj->map, (cstr_t)name);
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
  pmap_free(cstr_t)(buf->b_extmark_ns);
  /* FOR_ALL_EXTMARKS(buf) // wasn't required beforestoring pointers in kbtree */
    /* kb_del(extmarks, extmark); */
  /* END_FOR_ALL_EXTMARKS */
  kb_destroy(extmarks, buf->b_extmarks);
}

//TODO  use from mark.c
#define _col_adjust(pp) \
  { \
    pos_T *posp = pp; \
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
  FOR_ALL_EXTMARKS(buf)
    _col_adjust(&(extmark->mark))
  END_FOR_ALL_EXTMARKS
}

 /* Adjust extmark row for inserted/deleted rows. */
void extmark_adjust(buf_T* buf, linenr_T line1, linenr_T line2, long amount, long amount_after)
{
  FOR_ALL_EXTMARKS(buf)
    if (extmark->mark.lnum >= line1
        && extmark->mark.lnum <= line2) {
          if (amount == MAXLNUM) {
            extmark->mark.lnum = line1;
          }
          else {
            extmark->mark.lnum += amount;
          }
    }
    else if (extmark->mark.lnum > line2)
        extmark->mark.lnum += amount_after;
  END_FOR_ALL_EXTMARKS
}


/* static pos_T gen_relative(buf_T *buf, char *ns, linenr_T row, colnr_T col); */
/* { */
  /* pos_T pos; */
  /* pos.lnum = row; */
  /* pos.col = col; */
  // CALCULATE ABSPOSITION
  /* POS_T abs; */
  /* FOR_ALL_EXTMARKS_WITH_PREV(buf) */
      /* abs = extmark->mark; */
  /* FOR_ALL_EXTMARKS_WITH_PREV */
  /* return pos; */
/* } */


