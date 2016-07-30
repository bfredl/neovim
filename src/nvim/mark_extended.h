#ifndef NVIM_EXTMARK_H
#define NVIM_EXTMARK_H

#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"
#include "nvim/map.h"

#define extmark_pos_cmp(a, b) (pos_cmp((a)->mark, (b)->mark))

#define FOR_EXTMARKS_IN_NS(buf, ns) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  if (buf->b_extmarks) { \
    ExtmarkNs *ns_obj = pmap_get(uint64_t)(buf->b_extmark_ns, (uint64_t)ns); \
    if (ns_obj) { \
      kb_itr_first(extmarks, ns_obj->tree, &itr); \
      for (;kb_itr_valid(&itr); kb_itr_next(extmarks, ns_obj->tree, &itr)){ \
        extmark = kb_itr_key(ExtendedMarkPtr, &itr); \

#define END_FOR_EXTMARKS_IN_NS }}}

#define FOR_ALL_EXTMARKS(buf) \
  kbitr_t itr; \
  ExtendedMark *extmark; \
  if (buf->b_extmarks) { \
    for (kb_itr_first(extmarks, buf->b_extmarks, &itr); kb_itr_valid(&itr); kb_itr_next(extmarks, buf->b_extmarks, &itr)) { \
             extmark = kb_itr_key(ExtendedMarkPtr, &itr); \

#define END_FOR_ALL_EXTMARKS }}

typedef struct ExtendedMark {
  uint64_t id;
  /* kvec_t(uint64_t) id; */
  pos_T mark;
} ExtendedMark, *ExtendedMarkPtr;

typedef kvec_t(ExtendedMark*) ExtmarkArray;
typedef PMap(uint64_t) IntMap;
KBTREE_INIT(extmarks, ExtendedMarkPtr, extmark_pos_cmp)

/* All namesspace that exist in nvim */
typedef Map(cstr_t, uint64_t) _NS;
_NS *NAMESPACES;

typedef struct ExtmarkNs {
  IntMap *map;
  kbtree_t(extmarks) *tree;
} ExtmarkNs;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_EXTMARK_H
