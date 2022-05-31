#ifndef NVIM_MSGPACK_RPC_CONVERTER_H
#define NVIM_MSGPACK_RPC_CONVERTER_H

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "nvim/api/private/dispatch.h"
#include "nvim/api/private/helpers.h"
#include "mpack/mpack_core.h"
#include "mpack/object.h"

typedef struct {
  mpack_parser_t parser;
  mpack_tokbuf_t reader;
  size_t method_name_len;
  MsgpackRpcRequestHandler handler;
  Object result; // arg list or result
  Object error; // error return

  const char *read_ptr;
  size_t read_size;

  int state; // basic bitch state machine™
  MessageType type;
  uint32_t request_id;
} Unpacker;


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "msgpack_rpc/converter.h.generated.h"
#endif

#endif  // NVIM_MSGPACK_RPC_CONVERTER_H
