#include "nvim/lib/multihash.h"
#include "nvim/memory.h"

#define MH_TABLE MH_NAME()
#define MH_KEY MH_NAME(_key)

// INNER get, exceptions:
// rv == h->n_buckets: rare, but not found (completely full table)
// is_either(hash[rv]) : not found, but this is the place to put
// otherwise: hash[rv] is index into key/value arrays
//
// t->keys can be null if h->hash is empty!
uint32_t MH_NAME(_get)(MH_TABLE *t, MH_KEY key)
{
  MultiHashTab *h = &t->h;
  if (h->n_buckets == 0) {
    return 0;
  }
  uint32_t step = 0;
  uint32_t mask = h->n_buckets - 1;
  uint32_t k = MH_NAME(_hash)(key);
  uint32_t i = k & mask;
  uint32_t last = i;
  uint32_t site = h->n_buckets;
  while (!mh_is_empty(h, i)) {
    if (mh_is_del(h, i)) {
      site = i;
    } else if (MH_NAME(_equal)(t->keys[h->hash[i]], key)) {
      return i;
    }
    i = (i + (++step)) & mask;
    if (i == last) {
      return site;
    }
  }
  return site;
}

void MH_NAME(_rehash)(MH_TABLE *t)
{
  for (uint32_t k = 1; k < t->h.next_id; k++) {
    uint32_t idx = MH_NAME(_get)(t, t->keys[k]);
    if (mh_is_either((&t->h), idx)) {
      abort();
    }
    t->h.hash[idx] = k;
  }
}

uint32_t MH_NAME(_put)(MH_TABLE *t, MH_KEY key, MhPutStatus *new)
{
  MultiHashTab *h = &t->h;
  // might rehash ahead of time if "key" already existed. But it was
  // going to happen soon anyway.
  if (h->n_occupied >= h->upper_bound) {
    // TODO: check shrink/ just clear if a lot of tombstones
    mh_realloc(h, h->n_buckets + 1);
    MH_NAME(_rehash)(t);
  }

  uint32_t idx = MH_NAME(_get)(t, key);

  if (mh_is_either(h, idx)) {
    uint32_t pos = h->next_id++;
    if (pos >= h->n_keys) {
      h->n_keys = h->n_keys * 2;
      t->keys = xrealloc(t->keys, h->n_keys * sizeof(MH_KEY));
      // caller should realloc any value array
      *new = kMHNewKeyRealloc;
    } else {
      *new = kMHNewKeyDidFit;
    }
    h->hash[idx] = pos;
    return pos;
  } else {
    *new = kMHExisting;
    return h->hash[idx];
  }
}

#undef MH_NAME
