#ifndef NVIM_MAP_H
#define NVIM_MAP_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nvim/api/private/defs.h"
#include "nvim/assert.h"
#include "nvim/highlight_defs.h"
#include "nvim/types.h"

#if defined(__NetBSD__)
# undef uint64_t
# define uint64_t uint64_t
#endif

typedef const char *cstr_t;
typedef void *ptr_t;

#define Set(type) Set_##type
#define Map(T, U) Map_##T##U
#define PMap(T) Map(T, ptr_t)

static const int value_init_int = 0;
static const ptr_t value_init_ptr_t = NULL;
static const ssize_t value_init_ssize_t = -1;
static const uint32_t value_init_uint32_t = 0;
static const uint64_t value_init_uint64_t = 0;
static const String value_init_String = STRING_INIT;
static const ColorItem value_init_ColorItem = COLOR_ITEM_INITIALIZER;

// layer 0: type non-specific code

typedef struct {
  uint32_t n_buckets;
  uint32_t size;
  uint32_t n_occupied;
  uint32_t upper_bound;
  uint32_t n_keys;  // this is almost always "size", but keys[] could contain ded items..
  uint32_t keys_capacity;
  uint32_t *hash;
} HashTab;

#define MULTIHASHTAB_INIT { 0, 0, 0, 0, 0, 0, NULL }
#define SET_INIT { MULTIHASHTAB_INIT, NULL }
#define MAP_INIT { SET_INIT, NULL }

#define MH_TOMBSTONE UINT32_MAX

#define mh_is_empty(h, i) ((h)->hash[i] == 0)
#define mh_is_del(h, i) ((h)->hash[i] == MH_TOMBSTONE)
#define mh_is_either(h, i) ((uint32_t)((h)->hash[i]+1U) <= 1U)

typedef enum {
  kMHExisting = 0,
  kMHNewKeyDidFit,
  kMHNewKeyRealloc,
} MhPutStatus;

static inline uint32_t mh_unhash(HashTab *h, uint32_t idx)
{
  // It did the dead
  uint32_t pos = h->hash[idx]-1;
  h->hash[idx] = MH_TOMBSTONE;
  return pos;
}

void mh_clear(HashTab *h);
void mh_realloc(HashTab *h, uint32_t n_min_buckets);

// layer 1: key type specific defs
// This is all need for sets.

#define KEY_DECLS(T) \
  typedef struct { \
    HashTab h; \
    T *keys; \
  } Set(T); \
  \
  uint32_t mh_find_bucket_##T(Set(T) *t, T key, bool put); \
  uint32_t mh_get_##T(Set(T) *t, T key); \
  void mh_rehash_##T(Set(T) *t); \
  uint32_t mh_put_##T(Set(T) *t, T key, MhPutStatus *new); \
  uint32_t mh_delete_##T(Set(T) *t, uint32_t kidx); \
  \
  static inline bool set_put_##T(Set(T) *set, T key, T **key_alloc) { \
    MhPutStatus status; \
    uint32_t k = mh_put_##T(set, key, &status); \
    if (key_alloc) { \
      *key_alloc = &set->keys[k]; \
    } \
    return status != kMHExisting; \
  } \
  static inline void set_del_##T(Set(T) *set, T key) \
  { \
    uint32_t k = mh_get_##T(set, key); \
    if (k != MH_TOMBSTONE) { \
      /* TODO: get k out of set->keys */ \
      k = mh_unhash(&set->h, k); \
    } \
  } \
  static inline bool set_has_##T(Set(T) *set, T key) { \
    return mh_get_##T(set, key) != MH_TOMBSTONE; \
  } \

// layer 2: key+value specific defs
// now we finally get Maps

#define MAP_DECLS(T, U) \
  typedef struct { \
    Set_##T t; \
    U *values; \
  } Map(T, U); \
  static inline U map_get_##T##U(Map(T, U) *map, T key) \
  { \
    uint32_t k = mh_get_##T(&map->t, key); \
    return k == MH_TOMBSTONE ? value_init_##U : map->values[k]; \
  } \
  U map_get_##T##U(Map(T, U) *map, T key); \
  void map_put_##T##U(Map(T, U) *map, T key, U value); \
  U *map_ref_##T##U(Map(T, U) *map, T key, T **key_alloc); \
  U *map_put_ref_##T##U(Map(T, U) *map, T key, T **key_alloc, bool *new_item); \
  U map_del_##T##U(Map(T, U) *map, T key, T *key_alloc); \

// NOTE: Keys AND values must be allocated! Map and Set does not make a copy.

#define quasiquote(x,y) x##y

KEY_DECLS(int)
KEY_DECLS(cstr_t)
KEY_DECLS(ptr_t)
KEY_DECLS(uint64_t)
KEY_DECLS(uint32_t)
KEY_DECLS(String)
KEY_DECLS(HlEntry)
KEY_DECLS(ColorKey)

MAP_DECLS(int, int)
MAP_DECLS(int, ptr_t)
MAP_DECLS(cstr_t, ptr_t)
MAP_DECLS(cstr_t, int)
MAP_DECLS(ptr_t, ptr_t)
MAP_DECLS(uint32_t, ptr_t)
MAP_DECLS(uint64_t, ptr_t)
MAP_DECLS(uint64_t, ssize_t)
MAP_DECLS(uint64_t, uint64_t)
MAP_DECLS(uint32_t, uint32_t)
MAP_DECLS(HlEntry, int)
MAP_DECLS(String, int)
MAP_DECLS(int, String)
MAP_DECLS(ColorKey, ColorItem)

#define set_has(T, set, key) set_has_##T(set, key)
#define set_put(T, set, key) set_put_##T(set, key, NULL)
#define set_put_ref(T, set, key, key_alloc) set_put_##T(set, key, key_alloc)
#define set_del(T, set, key) set_del_##T(set, key)
#define set_destroy(T, set) (xfree((set)->keys), xfree((set)->h.hash))
#define set_clear(T, set) mh_clear(&(set)->h)
#define set_size(set) ((set)->h.size)

#define map_get(T, U) map_get_##T##U
#define map_has(T, map, key) set_has(T, &(map)->t, key)
#define map_put(T, U) map_put_##T##U
#define map_ref(T, U) map_ref_##T##U
#define map_put_ref(T, U) map_put_ref_##T##U
#define map_del(T, U) map_del_##T##U
#define map_destroy(T, map) (set_destroy(T, &(map)->t), xfree((map)->values))
#define map_clear(T, map) set_clear(T, &(map)->t)
#define map_size(map) ((map)->t.h.size)

#define pmap_get(T) map_get(T, ptr_t)
#define pmap_put(T) map_put(T, ptr_t)
#define pmap_ref(T) map_ref(T, ptr_t)
#define pmap_put_ref(T) map_put_ref(T, ptr_t)
/// @see pmap_del2
#define pmap_del(T) map_del(T, ptr_t)

#define set_foreach(set, key, block) \
  { \
    uint32_t __i; \
    for (__i = 0; __i < (set)->h.n_keys; __i++) { \
      (key) = (set)->keys[__i]; \
      block; \
    } \
  }

#define map_foreach_key(map, key, block) set_foreach(&(map)->t, key, block)

#define map_foreach(map, key, value, code) \
  { \
    uint32_t __i; \
    for (__i = 0; __i < (map)->t.h.n_keys; __i++) { \
      (key) = (map)->t.keys[__i]; \
      (value) = (map)->values[__i]; \
      code; \
    } \
  }

#define map_foreach_value(map, value, code) \
  { \
    uint32_t __i; \
    for (__i = 0; __i < (map)->t.h.n_keys; __i++) { \
      (value) = (map)->values[__i]; \
      code; \
    } \
  }


void pmap_del2(PMap(cstr_t) *map, const char *key);

#endif  // NVIM_MAP_H
