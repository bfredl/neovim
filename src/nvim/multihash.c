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
  uint32_t *hash;
} MultiHashTab;

#define HASH_EMPTY UINT32_MAX

#define is_empty(h, i) (h->hash[i] == 0)
#define is_del(h, i) (h->hash[i] == HASH_EMPTY)
#define is_either(h, i) ((uint32_t)(h->hash[i]+1U) <= 1U)

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

void kh_clear(MultiHashTab *h)
{
  if (h->hash) {
    memset(h->hash, 0, h->n_buckets * sizeof(*h->hash));
    h->size = h->n_occupied = 0;
  }
}

// INNER get, exceptions:
// rv == h->n_buckets: rare, but not found (completely full table)
// is_either(hash[rv]) : not found, but this is the place to put
// otherwise: hash[rv] is index into key/value arrays
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
    } else if (testkey_equal(keys[i], key)) {
      return i;
    }
    i = (i + (++step)) & mask;
    if (i == last) {
      return site;
    }
  }
  return site;
}

// TODO: this should be _less_ for the h->hash part as this is now small (4 bytes per value)
#define UPPER_FILL 0.77

void mh_alloc(MultiHashTab *h, uint32_t n_buckets) {
  // sets all buckets to EMPTY
  h->hash = calloc(n_buckets, sizeof *h->hash);
  h->n_buckets = n_buckets;
  h->n_occupied = h->size = 0;
  h->upper_bound = (uint32_t)(h->n_buckets * UPPER_FILL + 0.5);
  h->n_deleted = 0;
}

void mh_put(MultiHashTab *h, testkey **keys, uint32_t n_buckets) {
}

# define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, \
                        ++(x))

void kh_resize(MultiHashTab *h, uint32_t new_n_buckets, size_t val_size)
{  /* This function uses 0.25*n_buckets bytes of working space instead of */
   /* [sizeof(key_t+val_t)+.25]*n_buckets. */
  uint32_t *new_hash = 0;
  bool rehash = true;
  {
    kroundup32(new_n_buckets);
    if (new_n_buckets < 4) {
      new_n_buckets = 4;
    }
    /* requested size is too small */
    if (h->size >= (uint32_t)(new_n_buckets * UPPER_FILL + 0.5)) {
      rehash = false;
    } else {  /* hash table size to be changed (shrink or expand); rehash */
      new_hash = (uint32_t *)malloc(new_n_buckets * sizeof(uint32_t));
      memset(new_hash, 0xaa, new_n_buckets * sizeof(uint32_t));
      if (h->n_buckets < new_n_buckets) {  /* expand */
        /*
        khkey_t *new_keys = (khkey_t *)krealloc((void *)h->keys, new_n_buckets * sizeof(khkey_t));
        h->keys = new_keys;
        if (val_size) {
          char *new_vals = krealloc( h->vals_buf, new_n_buckets * val_size);
          h->vals_buf = new_vals;
        }
        */
      }  /* otherwise shrink */
    }
  }

  char cval[1];
  char ctmp[1];
  if (rehash) {  /* rehashing is needed */
    for (int j = 0; j != h->n_buckets; ++j) {
      if (is_either(h, j) == 0) {
        testkey key = keys[j];
        uint32_t new_mask;
        new_mask = new_n_buckets - 1;
        if (val_size) {
          kh_copyval(cval, kh_bval(h, j));
        }
        __ac_set_isdel_true(h, j);
        /* kick-out process; sort of like in Cuckoo hashing */
        while (1) {
          uint32_t k, i, step = 0;
          k = __hash_func(key);
          i = k & new_mask;
          while (!__ac_isempty(new_flags, i)) {
            i = (i + (++step)) & new_mask;
          }
          __ac_set_isempty_false(new_flags, i);
          /* kick out the existing element */
          if (i < h->n_buckets && is_either(h, i) == 0) {
            {
              khkey_t tmp = h->keys[i];
              h->keys[i] = key;
              key = tmp;
            }
            if (val_size) {
              kh_copyval(ctmp, kh_bval(h, i));
              kh_copyval(kh_bval(h, i), cval);
              kh_copyval(cval, ctmp);
            }
            /* mark it as deleted in the old hash table */
            __ac_set_isdel_true(h, i);
          } else {  /* write the element and jump out of the loop */
            h->keys[i] = key;
            if (val_size) {
              kh_copyval(kh_bval(h, i), cval);
            }
            break;
          }
        }
      }
    }
    if (h->n_buckets > new_n_buckets) {  /* shrink the hash table */
      h->keys = (khkey_t *)krealloc((void *)h->keys,
                                    new_n_buckets * sizeof(khkey_t));
      if (val_size) {
        h->vals_buf = krealloc((void *)h->vals_buf, new_n_buckets * val_size);
      }
    }
    kfree(h);  /* free the working space */
    h = new_flags;
    h->n_buckets = new_n_buckets;
    h->n_occupied = h->size;
    h->upper_bound = (uint32_t)(h->n_buckets * __ac_HASH_UPPER + 0.5);
  }
}
uint32_t kh_put(MultiHashTab *h, khkey_t key, int *ret, size_t val_size)
{
  uint32_t x;
  if (h->n_occupied >= h->upper_bound) {  /* update the hash table */
    if (h->n_buckets > (h->size << 1)) {
      kh_resize(h, h->n_buckets - 1, val_size);  /* clear "deleted" elements */
    } else {
      kh_resize(h, h->n_buckets + 1, val_size);  /* expand the hash table */
    }
  }  /* TODO: implement automatically shrinking; */
     /* resize() already support shrinking */
  {
    uint32_t k, i, site, last, mask = h->n_buckets - 1, step = 0;
    x = site = h->n_buckets;
    k = __hash_func(key);
    i = k & mask;
    if (__ac_isempty(h, i)) {
      x = i;  /* for speed up */
    } else {
      last = i;
      while (!__ac_isempty(h, i)
             && (__ac_isdel(h, i)
                 || !__hash_equal(h->keys[i], key))) {
        if (__ac_isdel(h, i)) {
          site = i;
        }
        i = (i + (++step)) & mask;
        if (i == last) {
          x = site;
          break;
        }
      }
      if (x == h->n_buckets) {
        if (__ac_isempty(h, i) && site != h->n_buckets) {
          x = site;
        } else {
          x = i;
        }
      }
    }
  }
  if (__ac_isempty(h, x)) {  /* not present at all */
    h->keys[x] = key;
    __ac_set_isboth_false(h, x);
    h->size++;
    h->n_occupied++;
    *ret = 1;
  } else if (__ac_isdel(h, x)) {  /* deleted */
    h->keys[x] = key;
    __ac_set_isboth_false(h, x);
    h->size++;
    *ret = 2;
  } else {
    *ret = 0;  /* Don't touch h->keys[x] if present and not deleted */
  }
  return x;
}
void kh_del(MultiHashTab *h, uint32_t x)
{
  if (x != h->n_buckets && !is_either(h, x)) {
    __ac_set_isdel_true(h, x);
    --h->size;
  }
}

