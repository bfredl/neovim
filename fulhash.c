#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// PROTOTYE: hardcode cstr_t hash
typedef char *testkey;

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

#define HASH_TOMBSTONE UINT32_MAX

#define is_empty(h, i) (h->hash[i] == 0)
#define is_del(h, i) (h->hash[i] == HASH_TOMBSTONE)
#define is_either(h, i) ((uint32_t)(h->hash[i]+1U) <= 1U)

# define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, \
                        ++(x))


static inline uint32_t testkey_hash(const char *s)
{
  // TODO: lookup science!
  uint32_t h = (uint8_t)*s;
  if (h) {
    for (++s; *s; ++s) { h = (h << 5) - h + (uint8_t)*s; }
  }
  return h;
}

static inline bool testkey_equal(const char *s1, const char *s2)
{
  return strcmp(s1, s2) == 0;
}

void mh_alloc(MultiHashTab *h, uint32_t n_buckets) {
  // sets all buckets to EMPTY
  h->hash = calloc(n_buckets, sizeof *h->hash);
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
    h->next_id = 1;
  }
}

// INNER get, exceptions:
// rv == h->n_buckets: rare, but not found (completely full table)
// is_either(hash[rv]) : not found, but this is the place to put
// otherwise: hash[rv] is index into key/value arrays
//
// keys can be null if h->hash is empty!
uint32_t mh_get_inner(const MultiHashTab *h, testkey *keys, testkey key)
{
  if (h->n_buckets == 0) {
    return 0;
  }
  uint32_t step = 0;
  uint32_t mask = h->n_buckets - 1;
  uint32_t k = testkey_hash(key);
  uint32_t i = k & mask;
  uint32_t last = i;
  uint32_t site = h->n_buckets;
  while (!is_empty(h, i)) {
    if (is_del(h, i)) {
      site = i;
    } else if (testkey_equal(keys[h->hash[i]], key)) {
      return i;
    }
    i = (i + (++step)) & mask;
    if (i == last) {
      return site;
    }
  }
  return site;
}

void mh_rehash(MultiHashTab *h, testkeys *keys, uint32_t new_n_buckets)
{
  // This function uses no working space (yet, needed if we both resize hash and keys array[s] at the same time)

  free(h->hash);
  kroundup32(new_n_buckets);
  mh_alloc(h, new_n_buckets);
  h->n_occupied = h->next_id;
  h->size = h->next_id;

  for (uint32_t k = 1; k < h->next_id; k++) {
    uint32_t idx = mh_get_inner(h, keys, keys[k]);
    if (is_either(h, idx)) {
      abort();
    }
    h->hash[idx] = k;
  }
}

// return: index in (*keys)[]
uint32_t mh_put(MultiHashTab *h, testkey **keys, testkey key)
{
  if (h->n_occupied >= h->upper_bound) {
    // TODO: check shrink if a lot of tombstones
    mh_realloc(h, h->n_buckets + 1);
  }

  uint32_t idx = mh_get_inner(h, *keys, key);

  if (is_empty(h, idx) && h->n_occupied >= h->upper_bound) {
    abort();
    mh_resize(h, *keys, h->n_buckets+1);
    idx = mh_get_inner(h, *keys, key);
  }

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

void mh_del(MultiHashTab *h, uint32_t x)
{
  if (!is_either(h, x)) {
    h->hash[x] = HASH_TOMBSTONE;
    h->size--;
  }
}
