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
  uint32_t n_keys;  // this is almost always "size", but keys[] could contain ded items..
  uint32_t keys_capacity;
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
    for (__i = 0; __i < (t)->h.n_keys; __i++) { \
      (kvar) = (t)->keys[__i]; \
      code; \
    } \
  }

#define MULTI_HASH_DECLS(k) \
  typedef struct { \
    MultiHashTab h; \
    k *keys; \
  } MHTable_##k; \
  \
  uint32_t mh_get_##k(MHTable_##k *t, k key); \
  void mh_rehash_##k(MHTable_##k *t); \
  uint32_t mh_put_##k(MHTable_##k *t, k key, MhPutStatus *new); \
  uint32_t mh_unhash_##k(MHTable_##k *t, k key); \

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "lib/multihash.h.generated.h"
#endif

#endif
