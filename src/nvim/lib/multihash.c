// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "nvim/lib/multihash.h"
#include "nvim/memory.h"
#include "nvim/strings.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "lib/multihash.c.generated.h"
#endif

// this should be _less_ for the h->hash part as this is now small (4 bytes per value)
#define UPPER_FILL 0.77

// h->hash must either be NULL or an already valid pointer
void mh_realloc(MultiHashTab *h, uint32_t n_min_buckets) {
  xfree(h->hash);
  uint32_t n_buckets = n_min_buckets < 16 ? 16 : n_min_buckets;
  kroundup32(n_buckets);
  // sets all buckets to EMPTY
  h->hash = xcalloc(n_buckets, sizeof *h->hash);
  h->n_occupied = h->size = 0;
  h->n_buckets = n_buckets;
  h->upper_bound = (uint32_t)(h->n_buckets * UPPER_FILL + 0.5);
  h->n_deleted = 0;
}

void mh_clear(MultiHashTab *h)
{
  if (h->hash) {
    memset(h->hash, 0, h->n_buckets * sizeof(*h->hash));
    h->size = h->n_occupied = 0;
    h->n_deleted = 0;
    h->n_keys = 0;
  }
}
