#include "nvim/lib/multihash.h"

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
uint32_t MH_NAME(get)(const MultiHashTab *h, MH_NAME(key) *keys, MH_NAME(key) key)
{
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
    } else if (MH_NAME(equal)(keys[h->hash[i]], key)) {
      return i;
    }
    i = (i + (++step)) & mask;
    if (i == last) {
      return site;
    }
  }
  return site;
}

void MH_NAME(rehash)(MultiHashTab *h, MH_NAME(key) *keys)
{
  for (uint32_t k = 1; k < h->next_id; k++) {
    uint32_t idx = MH_NAME(get)(h, keys, keys[k]);
    if (mh_is_either(h, idx)) {
      abort();
    }
    h->hash[idx] = k;
  }
}

uint32_t MH_NAME(put)(MultiHashTab *h, testkey **keys, testkey key)
{
  if (h->n_occupied >= h->upper_bound) {
    // TODO: check shrink if a lot of tombstones
    mh_realloc(h, h->n_buckets + 1);
    mh_rehash(h, keys);
  }

  uint32_t idx = mh_get_inner(h, *keys, key);

  if (is_either(h, idx)) {
    uint32_t pos = h->next_id++;
    if (pos >= h->n_keys) {
      abort();
    }
    h->hash[idx] = pos;
    return pos;
  } else {
    return h->hash[idx];
  }
}

#undef MH_NAME
