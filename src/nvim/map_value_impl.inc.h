#include "nvim/map.h"
#include "nvim/assert.h"

#if !defined(KEY_NAME) || !defined(VAL_NAME)
// Don't error out. it is nice to type-check the file in isolation, in clangd or otherwise
#define KEY_NAME(x) x##int
#define VAL_NAME(x) quasiquote(x, ptr_t)
#endif

#define MAP_NAME(x) VAL_NAME(KEY_NAME(x))
#define MAP_TYPE MAP_NAME(Map_)
#define KEY_TYPE KEY_NAME()
#define VALUE_TYPE VAL_NAME()
#define INITIALIZER VAL_NAME(value_init_)

void MAP_NAME(map_put_)(MAP_TYPE *map, KEY_TYPE key, VALUE_TYPE value)
{
  MhPutStatus status;
  uint32_t k = KEY_NAME(mh_put_)(&map->t, key, &status);
  if (status == kMHNewKeyRealloc) {
    map->values = xrealloc(map->values, map->t.h.keys_capacity*sizeof(VALUE_TYPE));
  }
  map->values[k] = value;
}

VALUE_TYPE *MAP_NAME(map_ref_)(MAP_TYPE *map, KEY_TYPE key, KEY_TYPE **key_alloc)
{
  uint32_t k = KEY_NAME(mh_get_)(&map->t, key);
  if (k == MH_TOMBSTONE) {
    return NULL;
  }
  if (key_alloc) {
    *key_alloc = &map->t.keys[k];
  }
  return &map->values[k];
}

VALUE_TYPE *MAP_NAME(map_put_ref_)(MAP_TYPE *map, KEY_TYPE key, KEY_TYPE **key_alloc, bool *new_item)
{
  MhPutStatus status;
  uint32_t k = KEY_NAME(mh_put_)(&map->t, key, &status);
  if (status != kMHExisting) {
    if (status == kMHNewKeyRealloc) {
      map->values = xrealloc(map->values, map->t.h.keys_capacity*sizeof(VALUE_TYPE));
    }
     map->values[k] = INITIALIZER;
  }
  if (new_item) {
    *new_item = (status != kMHExisting);
  }
  if (key_alloc) {
    *key_alloc = &map->t.keys[k];
  }
  return &map->values[k];
}

VALUE_TYPE MAP_NAME(map_del_)(MAP_TYPE *map, KEY_TYPE key, KEY_TYPE *key_alloc)
{
  VALUE_TYPE rv = INITIALIZER;
  if (map->t.h.size == 0) {
    return rv;
  }
  uint32_t idx = KEY_NAME(mh_find_bucket_)(&map->t, key, false);
  if (idx != MH_TOMBSTONE) {
    uint32_t k = mh_unhash(&map->t.h, idx);
    rv = map->values[k];
    if (key_alloc) {
      *key_alloc = map->t.keys[k];
    }
    uint32_t knew = KEY_NAME(mh_delete_)(&map->t, k);
    if (knew != MH_TOMBSTONE) {
      map->values[k] = map->values[knew];
    }
  }
  return rv;
}
