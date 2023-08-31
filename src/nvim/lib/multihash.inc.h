#include "nvim/lib/multihash.h"

#define MH_PREFIX foo_

#define MH_NAME(x) foo_ ## x

typedef char * MH_NAME(key);

typedef struct {
  MultiHashTab h;
  MH_NAME(key) *keys;
} MH_NAME(hashtab);

typedef enum {
  kMHExisting = 0,
  kMHNewKeyDidFit,
  kMHNewKeyRealloc,
} MhPutStatus;

