#include "nvim/lib/multihash.h"
#include "nvim/lib/multihash.inc.h"
#include "nvim/memory.h"

#define MH_PREFIX foo_
#define MH_NAME(x) foo_ ## x

static inline uint32_t MH_NAME(hash)(const char *s)
{
  // TODO: lookup science!
  uint32_t h = (uint8_t)*s;
  if (h) {
    for (++s; *s; ++s) { h = (h << 5) - h + (uint8_t)*s; }
  }
  return h;
}

static inline bool MH_NAME(equal)(const char *s1, const char *s2)
{
  return strcmp(s1, s2) == 0;
}

// INNER get, exceptions:
// rv == h->n_buckets: rare, but not found (completely full table)
// is_either(hash[rv]) : not found, but this is the place to put
// otherwise: hash[rv] is index into key/value arrays
//
// keys can be null if h->hash is empty!
uint32_t MH_NAME(get)(MH_NAME(hashtab) *t, MH_NAME(key) key)
{
  MultiHashTab *h = &t->h;
  if (h->n_buckets == 0) {
    return 0;
  }
  uint32_t step = 0;
  uint32_t mask = h->n_buckets - 1;
  uint32_t k = MH_NAME(hash)(key);
  uint32_t i = k & mask;
  uint32_t last = i;
  uint32_t site = h->n_buckets;
  while (!mh_is_empty(h, i)) {
    if (mh_is_del(h, i)) {
      site = i;
    } else if (MH_NAME(equal)(t->keys[h->hash[i]], key)) {
      return i;
    }
    i = (i + (++step)) & mask;
    if (i == last) {
      return site;
    }
  }
  return site;
}

void MH_NAME(rehash)(MH_NAME(hashtab) *t)
{
  for (uint32_t k = 1; k < t->h.next_id; k++) {
    uint32_t idx = MH_NAME(get)(t, t->keys[k]);
    if (mh_is_either((&t->h), idx)) {
      abort();
    }
    t->h.hash[idx] = k;
  }
}

uint32_t MH_NAME(put)(MH_NAME(hashtab) *t, MH_NAME(key) key, MhPutStatus *new)
{
  MultiHashTab *h = &t->h;
  // might rehash ahead of time if "key" already existed. But it was
  // going to happen soon anyway.
  if (h->n_occupied >= h->upper_bound) {
    // TODO: check shrink/ just clear if a lot of tombstones
    mh_realloc(h, h->n_buckets + 1);
    MH_NAME(rehash)(t);
  }

  uint32_t idx = MH_NAME(get)(t, key);

  if (mh_is_either(h, idx)) {
    uint32_t pos = h->next_id++;
    if (pos >= h->n_keys) {
      h->n_keys = h->n_keys * 2;
      t->keys = xrealloc(t->keys, h->n_keys * sizeof(MH_NAME(key)));
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
