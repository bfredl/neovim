#ifndef NVIM_LIB_MULTIHASH_H
#define NVIM_LIB_MULTIHASH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t n_buckets;
  uint32_t size;
  uint32_t n_occupied;
  uint32_t n_deleted;
  uint32_t upper_bound;
  uint32_t next_id;  // TODO: more like actual n_keys!
  uint32_t n_keys;  // TODO: more like capacity
  uint32_t *hash;
} MultiHashTab;

#define MULTIHASHTAB_INIT { 0, 0, 0, 0, 0, 0, 0, NULL }
#define MULTIHASH_SET_INIT { MULTIHASHTAB_INIT, NULL }

#define MH_TOMBSTONE UINT32_MAX

#define mh_is_empty(h, i) ((h)->hash[i] == 0)
#define mh_is_del(h, i) ((h)->hash[i] == MH_TOMBSTONE)
#define mh_is_either(h, i) ((uint32_t)((h)->hash[i]+1U) <= 1U)

# define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, \
                        ++(x))
typedef enum {
  kMHExisting = 0,
  kMHNewKeyDidFit,
  kMHNewKeyRealloc,
} MhPutStatus;

static inline uint32_t mh_unhash(MultiHashTab *h, uint32_t idx)
{
  // It did the dead
  uint32_t pos = h->hash[idx];
  h->hash[idx] = MH_TOMBSTONE;
  return pos;
}

#define mh_foreach_key(t, kvar, code) \
  { \
    uint32_t __i; \
    for (__i = 0; __i < (t)->h.next_id; __i++) { \
      (kvar) = (t)->keys[__i]; \
      code; \
    } \
  }

#define MULTI_HASH_DECLS(name, T) \
  typedef T name##_key; \
  typedef struct { \
    MultiHashTab h; \
    name##_key *keys; \
  } name; \
  \
  uint32_t name##_get(name *t, name##_key key); \
  void name##_rehash(name *t); \
  uint32_t name##_put(name *t, name##_key key, MhPutStatus *new); \
  uint32_t name##_unhash(name *t, name##_key key); \

MULTI_HASH_DECLS(FooTable, char *)

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "lib/multihash.h.generated.h"
#endif

#endif
