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
  uint32_t next_id;
  uint32_t n_keys;
  uint32_t *hash;
} MultiHashTab;

#define MULTIHASHTAB_INIT { 0, 0, 0, 0, 0, 0, 0, NULL }

#define HASH_TOMBSTONE UINT32_MAX

#define mh_is_empty(h, i) ((h)->hash[i] == 0)
#define mh_is_del(h, i) ((h)->hash[i] == HASH_TOMBSTONE)
#define mh_is_either(h, i) ((uint32_t)((h)->hash[i]+1U) <= 1U)

# define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, \
                        ++(x))


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "lib/multihash.h.generated.h"
#endif

#endif
