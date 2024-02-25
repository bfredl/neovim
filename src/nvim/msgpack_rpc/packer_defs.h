#pragma once

#include <stddef.h>
#include <stdint.h>

#define MPACK_ITEM_SIZE 9

typedef struct packer_buffer_t PackerBuffer;

// Must ensure at least MPACK_ITEM_SIZE of space.
typedef void (*PackerBufferFlush)(PackerBuffer *self);

struct packer_buffer_t {
  char *startptr;
  char *ptr;
  char *endptr;

  // these are free to use by flush for any purpose
  void *anydata;
  size_t anylen;
  PackerBufferFlush packer_flush;
};
