#ifndef NVIM_EXTMARK_H
#define NVIM_EXTMARK_H

#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"
#include "nvim/map.h"

/* typedef struct ExtendedMark ExtendedMark; */
typedef struct ExtendedMark {
  char *name;
  pos_T mark;
} ExtendedMark, *ExtendedMarkPtr;

#define extmark_pos_cmp(a, b) (pos_cmp((a)->mark, (b)->mark))

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
  if (buf->b_extmarks) { \
    ExtmarkNamespace *ns_obj = pmap_get(cstr_t)(buf->b_extmark_ns, (cstr_t)ns); \
    kb_itr_first(extmarks, ns_obj->tree, &itr);\
    for (;kb_itr_valid(&itr); kb_itr_next(extmarks, ns_obj->tree, &itr)){\
      extmark = &kb_itr_key(ExtendedMark, &itr); \

#define END_FOR_EXTMARKS_IN_NS }}

#define FOR_ALL_EXTMARKS(buf) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  if (buf->b_extmarks) { \
    for (kb_itr_first(extmarks, buf->b_extmarks, &itr); \
         kb_itr_valid(&itr); \
         kb_itr_next(extmarks, buf->b_extmarks, &itr)) { \
             extmark = &kb_itr_key(ExtendedMark, &itr); \

#define END_FOR_ALL_EXTMARKS }}

typedef kvec_t(char *) ExtmarkNames;
typedef kvec_t(ExtendedMark*) ExtmarkArray;
typedef PMap(cstr_t) ExtmarkNsMap;
KBTREE_INIT(extmarks, ExtendedMarkPtr, extmark_pos_cmp)

typedef struct ExtmarkNamespace {
  StringMap *map;
  kbtree_t(extmarks) *tree;
} ExtmarkNamespace;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_EXTMARK_H
