#ifndef NVIM_EXTMARK_H
#define NVIM_EXTMARK_H

#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"
#include "nvim/map.h"

typedef PMap(cstr_t) StringMap2; // TODO do these have to be seperate?

typedef struct ExtendedMark ExtendedMark;
struct ExtendedMark {
  StringMap2 *names; // key = namespace, value = id
  fmark_T fmark;
};

#define extmark_pos_cmp(a, b) (pos_cmp((a).fmark.mark, (b).fmark.mark))

/* prev pointer is used for accessing the previous extmark iterated over */
/* TODO there should be a better way to get this.. maybe build it in to kbtree */
/* exists checks if the extmark is there for a particular buffer namespace */
#define FOR_ALL_EXTMARKS_WITH_PREV(buf) \
  ExtendedMark *prev; \
  FOR_ALL_EXTMARKS(buf) \
    if (extmark) { \
      prev = extmark; \
    } \

#define END_FOR_ALL_EXTMARKS_WITH_PREV }}

#define FOR_EXTMARKS_IN_NS(buf, ns) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  char *_name; \
  if (buf->b_extmarks_tree) { \
    kb_itr_first(extmarks, buf->b_extmarks_tree, &itr);\
    for (;kb_itr_valid(&itr); kb_itr_next(extmarks, buf->b_extmarks_tree, &itr)){\
      extmark = &kb_itr_key(ExtendedMark, &itr); \
      _name = (char *)pmap_get(cstr_t)(extmark->names, ns); \
      if (!_name) { \
        continue; \
      }

#define END_FOR_EXTMARKS_IN_NS }}

#define FOR_ALL_EXTMARKS(buf) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  if (buf->b_extmarks_tree) { \
    for (kb_itr_first(extmarks, buf->b_extmarks_tree, &itr); \
         kb_itr_valid(&itr); \
         kb_itr_next(extmarks, buf->b_extmarks_tree, &itr)) { \
             extmark = &kb_itr_key(ExtendedMark, &itr); \

#define END_FOR_ALL_EXTMARKS }}

typedef kvec_t(char *) ExtmarkNames;
typedef kvec_t(ExtendedMark*) ExtmarkArray;
typedef PMap(cstr_t) ExtmarkNsMap;
KBTREE_INIT(extmarks, ExtendedMark, extmark_pos_cmp)
#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_EXTMARK_H
