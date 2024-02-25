#pragma once

#include <stddef.h>
#include <stdint.h>

#define MPACK_ITEM_SIZE 9

typedef struct packer_buffer_t PackerBuffer;

// MUST ensure at least MPACK_ITEM_SIZE of space. `size_hint` will be set to
// a larger value if we are packing a really large STR/BIN object (but fitting
// all of it is not required, it is just a hint to avoid a lot of flushes for
// one single object)
typedef void (*PackerBufferFlush)(PackerBuffer *self, size_t size_hint);

struct packer_buffer_t {
  char *startptr;
  char *ptr;
  char *endptr;

  // these are free to use by flush for any purpose
  void *anydata;
  size_t anylen;
  PackerBufferFlush flush;
};
