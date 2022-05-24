#ifndef NVIM_MSGPACK_RPC_CONVERTER_H
#define NVIM_MSGPACK_RPC_CONVERTER_H

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "nvim/api/private/helpers.h"
#include "mpack/mpack_core.h"
#include "mpack/object.h"

typedef struct {
  mpack_parser_t parser;
  Object *result;
  char fulbuffer[8192];
  size_t read
} Unpacker;


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "msgpack_rpc/converter.h.generated.h"
#endif

#endif  // NVIM_MSGPACK_RPC_CONVERTER_H
