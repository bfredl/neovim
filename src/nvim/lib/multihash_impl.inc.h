#include "nvim/lib/multihash.h"
#include "nvim/memory.h"

#ifndef MH_NAME
typedef int TESTING;
MULTI_HASH_DECLS(TESTING)
#define MH_NAME(x) x##TESTING
#endif

#define MH_TABLE MH_NAME(MHTable_)
#define MH_KEY MH_NAME()

// INNER get, exceptions:
// rv == h->n_buckets: rare, but not found (completely full table)
// is_either(hash[rv]) : not found, but this is the place to put
// otherwise: hash[rv] is index into key/value arrays
//
// t->keys can be null if h->hash is empty!
//
// @return bucket index!
uint32_t MH_NAME(mh_get_)(MH_TABLE *t, MH_KEY key)
{
  MultiHashTab *h = &t->h;
  if (h->n_buckets == 0) {
    return MH_TOMBSTONE;
  }
  uint32_t step = 0;
  uint32_t mask = h->n_buckets - 1;
  uint32_t k = MH_NAME(hash_)(key);
  uint32_t i = k & mask;
  uint32_t last = i;
  uint32_t site = i;
  while (!mh_is_empty(h, i)) {
    if (mh_is_del(h, i)) {
      site = i;
    } else if (MH_NAME(equal_)(t->keys[h->hash[i]-1], key)) {
      return i;
    }
    i = (i + (++step)) & mask;
    if (i == last) {
      return site;
    }
  }
  return site;
}

void MH_NAME(mh_rehash_)(MH_TABLE *t)
{
  for (uint32_t k = 0; k < t->h.n_keys; k++) {
    uint32_t idx = MH_NAME(mh_get_)(t, t->keys[k]);
    // there must be tombstones when we do a rehash
    if (!mh_is_empty((&t->h), idx)) {
      abort();
    }
    t->h.hash[idx] = k+1;
  }
  t->h.n_occupied = t->h.size = t->h.n_keys;
}

// @return keys index
uint32_t MH_NAME(mh_put_)(MH_TABLE *t, MH_KEY key, MhPutStatus *new)
{
  MultiHashTab *h = &t->h;
  // might rehash ahead of time if "key" already existed. But it was
  // going to happen soon anyway.
  if (h->n_occupied >= h->upper_bound) {
    // TODO: check shrink/ just clear if a lot of tombstones
    mh_realloc(h, h->n_buckets + 1);
    MH_NAME(mh_rehash_)(t);
  }

  uint32_t idx = MH_NAME(mh_get_)(t, key);

  if (mh_is_either(h, idx)) {
    h->size++;
    h->n_occupied++;
    uint32_t pos = h->n_keys++;
    if (pos >= h->keys_capacity) {
      h->keys_capacity = MAX(h->keys_capacity * 2, 8);
      t->keys = xrealloc(t->keys, h->keys_capacity * sizeof(MH_KEY));
      // caller should realloc any value array
      *new = kMHNewKeyRealloc;
    } else {
      *new = kMHNewKeyDidFit;
    }
    t->keys[pos] = key;
    h->hash[idx] = pos+1;
    return pos;
  } else {
    *new = kMHExisting;
    return h->hash[idx]-1;
  }
}

#undef MH_NAME
