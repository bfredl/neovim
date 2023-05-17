#include "nvim/map.h"
#include "nvim/memory.h"

#ifndef KEY_NAME
#define KEY_NAME(x) x##int
#endif

#define SET_TYPE KEY_NAME(Set_)
#define KEY_TYPE KEY_NAME()

/// find bucket to get or put "key"
///
/// t->h.hash assumed already allocated!
///
/// return conditions:
/// is_either(hash[rv]) : not found, but this is the place to put
/// otherwise: hash[rv]-1 is index into key/value arrays
///
/// @return bucket index
uint32_t KEY_NAME(mh_find_bucket_)(SET_TYPE *t, KEY_TYPE key, bool put)
{
  HashTab *h = &t->h;
  uint32_t step = 0;
  uint32_t mask = h->n_buckets - 1;
  uint32_t k = KEY_NAME(hash_)(key);
  uint32_t i = k & mask;
  uint32_t last = i;
  uint32_t site = put ? last : MH_TOMBSTONE;
  while (!mh_is_empty(h, i)) {
    if (mh_is_del(h, i)) {
      if (site == last) {
        site = i;
      }
    } else if (KEY_NAME(equal_)(t->keys[h->hash[i]-1], key)) {
      return i;
    }
    i = (i + (++step)) & mask;
    if (i == last) {
      abort();
    }
  }
  if (site == last) {
    site = i;
  }
  return site;
}

uint32_t KEY_NAME(mh_get_)(SET_TYPE *t, KEY_TYPE key)
{
  if (t->h.n_buckets == 0) {
    return MH_TOMBSTONE;
  }
  uint32_t idx = KEY_NAME(mh_find_bucket_)(t, key, false);
  return (idx !=  MH_TOMBSTONE) ? t->h.hash[idx] -1 : MH_TOMBSTONE;
}

void KEY_NAME(mh_rehash_)(SET_TYPE *t)
{
  for (uint32_t k = 0; k < t->h.n_keys; k++) {
    uint32_t idx = KEY_NAME(mh_find_bucket_)(t, t->keys[k], true);
    // there must be tombstones when we do a rehash
    if (!mh_is_empty((&t->h), idx)) {
      abort();
    }
    t->h.hash[idx] = k+1;
  }
  t->h.n_occupied = t->h.size = t->h.n_keys;
}

// @return keys index
uint32_t KEY_NAME(mh_put_)(SET_TYPE *t, KEY_TYPE key, MhPutStatus *new)
{
  HashTab *h = &t->h;
  // might rehash ahead of time if "key" already existed. But it was
  // going to happen soon anyway.
  if (h->n_occupied >= h->upper_bound) {
    // TODO: check shrink/ just clear if a lot of tombstones
    mh_realloc(h, h->n_buckets + 1);
    KEY_NAME(mh_rehash_)(t);
  }

  uint32_t idx = KEY_NAME(mh_find_bucket_)(t, key, true);

  if (mh_is_either(h, idx)) {
    h->size++;
    h->n_occupied++;
    uint32_t pos = h->n_keys++;
    if (pos >= h->keys_capacity) {
      h->keys_capacity = MAX(h->keys_capacity * 2, 8);
      t->keys = xrealloc(t->keys, h->keys_capacity * sizeof(KEY_TYPE));
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
    uint32_t pos = h->hash[idx]-1;
    if (!KEY_NAME(equal_)(t->keys[pos], key)) {
      abort();
    }
    return pos;
  }
}

uint32_t KEY_NAME(mh_delete_)(SET_TYPE *t, uint32_t kidx)
{
  uint32_t last = --t->h.n_keys;
  if (last != kidx) {
    uint32_t idx = KEY_NAME(mh_find_bucket_)(t, t->keys[last], true);
    if (t->h.hash[idx] != last+1) {
      abort();
    }
    t->h.hash[idx] = kidx + 1;
    t->keys[kidx] = t->keys[last];
    return last;
  }
  t->h.size--;
  return MH_TOMBSTONE;
}
