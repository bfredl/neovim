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
#include "nvim/map.h"          // pmap ...
#include "nvim/lib/kbtree.h"   // kbitr ...

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.c.generated.h"
#endif

static uint64_t NAMESPACE_COUNTER = 0;

/* required before calling any other functions */
uint64_t extmark_ns_create(char *ns)
{
  if (!NAMESPACE_COUNTER) {
    NAMESPACES = map_new(cstr_t, uint64_t)();
  }
  else if(map_has(cstr_t, uint64_t)(NAMESPACES, ns)) {
    return FAIL;
  }
  NAMESPACE_COUNTER++;
  uint64_t id = NAMESPACE_COUNTER;
  map_put(cstr_t, uint64_t)(NAMESPACES, xstrdup(ns), id);
  return NAMESPACE_COUNTER;
}

bool ns_initialized(char *ns) {
  if (!NAMESPACES) {
    return 0;
  }
  return map_has(cstr_t, uint64_t)(NAMESPACES, ns);
}


/* Create or update an extmark */
/* returns 1 on new mark created */
/* returns 2 on succesful update */
int extmark_set(buf_T *buf, uint64_t ns, uint64_t id, linenr_T row, colnr_T col)
{
  ExtendedMark *extmark = extmark_from_id(buf, ns, id);
  if (!extmark){
    return extmark_create(buf, ns, id, row, col);
  }
  else {
    extmark_update(extmark, buf, ns, id, row,  col);
   return 2;
  }
}

/* returns 0 on missing id */
int extmark_unset(buf_T *buf, uint64_t ns, uint64_t id)
{
  ExtendedMark *extmark = extmark_from_id(buf, ns, id);
  if (!extmark){
    return FAIL;
  }
  return extmark_delete(extmark, buf, ns, id);
}

/* Get all mark ids ordered by position*/ //TODO probably just use this directly in api
ExtmarkArray *extmark_ids(buf_T *buf, uint64_t ns)
{
  ExtmarkArray *array = xmalloc(sizeof(ExtmarkArray));
  kv_init(*array);
  FOR_EXTMARKS_IN_NS(buf, ns)
  /* FOR_ALL_EXTMARKS(buf) */
    kv_push(*array, extmark);
  END_FOR_EXTMARKS_IN_NS
  /* END_FOR_ALL_EXTMARKS */
  return array;
}

/* Given a mark, finds the next mark */
ExtendedMark *extmark_next(buf_T *buf, uint64_t ns, pos_T *pos, bool match)
{
  return extmark_neighbour(buf, ns, pos, 1, match);
}

/* Given a id finds the previous mark */
ExtendedMark *extmark_prev(buf_T *buf, uint64_t ns, pos_T *pos, bool match)
{
  return extmark_neighbour(buf, ns, pos, 0, match);
}

ExtmarkArray *extmark_nextrange(buf_T *buf, uint64_t ns, pos_T *lower, pos_T *upper)
{
  return extmark_neighbour_range(buf, ns, lower, upper, 1);
}

/* Returns the position of marks between a range, */
/* marks found at the start or end index will be included, */
/* if upper.lnum or upper.col are negative the buffer */
/* will be searched to the end, */
/* go_forward can be set to control the order of the array */
static ExtmarkArray *extmark_neighbour_range(buf_T *buf, uint64_t ns, pos_T *lower, pos_T *upper, bool go_forward)
{
  ExtmarkArray *array = xmalloc(sizeof(ExtmarkArray));
  kv_init(*array);
  int cmp;
  FOR_EXTMARKS_IN_NS(buf, ns)
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

/// Returns the mark, nearest to or including the passed in mark
/// @param input the desired position to be check agains
/// @param go_forward flag to control next or prev
/// @param match a flag to include a mark that is at the postiion being queried
static ExtendedMark *extmark_neighbour(buf_T *buf, uint64_t ns, pos_T *input, bool go_forward, bool match)
{
  int cmp;
  if (go_forward) {
    FOR_ALL_EXTMARKS(buf)
    /* FOR_EXTMARKS_IN_NS(buf, ns) */
      cmp = pos_cmp(*input, extmark->mark);
      if (cmp == -1) {
        return extmark;
      } else if (cmp == 0 && match) {
        return extmark;
      }
    /* END_FOR_EXTMARKS_IN_NS */
    END_FOR_ALL_EXTMARKS
  }
  /* else { */
  /* prev */
  /* } */
  return NULL;
}

ExtendedMark *extmark_ref(kbtree_t(extmarks) *b, pos_T pos, bool put)
{
  ExtendedMark t, *p , **pp;
  t.mark = pos;
  pp = kb_get(extmarks, b, &t);
  // IMPORTANT: put() only works if key is absent
  if (pp) {
      return *pp;
  } else if (!put) {
      return NULL;
  }
  p = xcalloc(sizeof(*p),1);
  p->mark = pos;
  // p->items zero initialized
  kb_put(extmarks, b, p);
  return p;
}

static bool extmark_create(buf_T *buf, uint64_t ns, uint64_t id, linenr_T row, colnr_T col)
{
  if (!buf->b_extmarks) {
    buf->b_extmarks = kb_init(extmarks, KB_DEFAULT_SIZE);
    buf->b_extmark_ns = pmap_new(uint64_t)();
  }
  /* ExtmarkNs *ns_obj = pmap_ref(uint64_t)(buf->b_extmark_ns, ns, true); */
  ExtmarkNs *ns_obj = NULL;
  ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
  /* Initialize a new namespace for this buffer*/
  if (!ns_obj) {
    ns_obj = xmalloc(sizeof(ExtmarkNs));
    ns_obj->map = pmap_new(uint64_t)();
    ns_obj->tree = kb_init(extmarks, KB_DEFAULT_SIZE);
    pmap_put(uint64_t)(buf->b_extmark_ns, ns, ns_obj);
  }

  /* ExtendedMark *extmark = xmalloc(sizeof(ExtendedMark)); */
  /* extmark->mark = gen_relative(buf, ns, row, col); */
  pos_T *pos = xcalloc(sizeof(pos_T), 1);
  pos->lnum=row;
  pos->col=col;
  /* extmark->mark.lnum=row;
   * extmark->mark.col=col; */

  ExtendedMark *extmark = extmark_ref(buf->b_extmarks, *pos, true);
  extmark->id = id;
  // extmark->mark = *pos;
  // kb_put(extmarks, buf->b_extmarks, extmark);
  kb_put(extmarks, ns_obj->tree, extmark);
  pmap_put(uint64_t)(ns_obj->map, id, extmark);

  return OK;
}

static void extmark_update(ExtendedMark *extmark, buf_T *buf, uint64_t ns, uint64_t id, linenr_T row, colnr_T col)
{
  extmark->mark.lnum = row;
  extmark->mark.col = col;
  /* gen_relative(); */
}

static int extmark_delete(ExtendedMark *extmark, buf_T *buf, uint64_t ns, uint64_t id)
{
  /* Remove our key from the namespace */
  ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
  pmap_del(uint64_t)(ns_obj->map, id);

  /* Delete the mark */
  kb_del(extmarks, buf->b_extmarks, extmark);
  kb_del(extmarks, ns_obj->tree, extmark);
  xfree(extmark); // wasn't required before change to storing pointers

  return OK;
}

ExtendedMark *extmark_from_id(buf_T *buf, uint64_t ns, uint64_t id)
{
  if (!buf->b_extmarks) {
    return NULL;
  }
  ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, ns);
  if (!ns_obj || !kh_size(ns_obj->map->table)) {
    return NULL;
  }
  return pmap_get(uint64_t)(ns_obj->map, id);
}

ExtendedMark *extmark_from_pos(buf_T *buf, uint64_t ns, linenr_T row, colnr_T col)
{
  if (!buf->b_extmarks) {
    return NULL;
  }
  pos_T pos;
  pos.lnum = row;
  pos.col = col;
  FOR_ALL_EXTMARKS(buf)
    if (pos_cmp(extmark->mark, pos) == 0) {
      return extmark;
    }
  END_FOR_ALL_EXTMARKS
  return NULL;
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
  pmap_free(uint64_t)(buf->b_extmark_ns);
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


/* static pos_T gen_relative(buf_T *buf, uint64_t ns, linenr_T row, colnr_T col); */
/* { */
  /* pos_T pos; */
  /* pos.lnum = row; */
  /* pos.col = col; */
  // CALCULATE ABSPOSITION
  /* POS_T abs; */
  /* FOR_ALL_EXTMARKS(buf) */
      /* abs = extmark->mark; */
  /* FOR_ALL_EXTMARKS */
  /* return pos; */
/* } */
