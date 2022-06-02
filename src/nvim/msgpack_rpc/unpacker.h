#ifndef NVIM_MSGPACK_RPC_UNPACKER_H
#define NVIM_MSGPACK_RPC_UNPACKER_H

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "mpack/mpack_core.h"
#include "mpack/object.h"
#include "nvim/api/private/dispatch.h"
#include "nvim/api/private/helpers.h"

struct consumed_blk {
  struct consumed_blk *prev;
};

#define ARENA_ALIGN sizeof(void*)
#define ARENA_BLOCK_SIZE 4096

typedef struct {
  char *cur_blk;
  size_t pos, size;
} MiniArena;

typedef struct {
  mpack_parser_t parser;
  mpack_tokbuf_t reader;

  const char *read_ptr;
  size_t read_size;

#define MAX_EXT_LEN 9  // byte + 8-byte integer
  char ext_buf[MAX_EXT_LEN];

  int state;
  MessageType type;
  uint32_t request_id;
  size_t method_name_len;
  MsgpackRpcRequestHandler handler;
  Object error;  // error return
  Object result;  // arg list or result
  Error unpack_error;

  MiniArena arena;
  // one lenght free-list of reusable blocks
  char *first_blk;
} Unpacker;


// unrecovareble error. unpack_error should be set!
#define unpacker_closed(p) ((p)->state < 0)

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "msgpack_rpc/unpacker.h.generated.h"
#endif

#endif  // NVIM_MSGPACK_RPC_UNPACKER_H
