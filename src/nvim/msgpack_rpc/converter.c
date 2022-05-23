// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "nvim/api/private/helpers.h"
#include "mpack/mpack_core.h"
#include "mpack/object.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "msgpack_rpc/converter.c.generated.h"
#endif


typedef struct {
  mpack_parser_t parser;
  Object *result;
} Unpacker;

Object convert(const char *data, size_t size, Error *err)
{
  Unpacker unpacker;
  Object rv = NIL;
  unpacker.result = &rv;
  mpack_parser_init(&unpacker.parser, 0);
  unpacker.parser.data.p = &unpacker;

  int result = mpack_parse(&unpacker.parser, &data, &size, api_parse_enter,
      api_parse_exit);

  if (result == MPACK_NOMEM) {
    api_set_error(err, kErrorTypeException, "object was too deep to unpack");
  } else if (result == MPACK_EOF) {
    api_set_error(err, kErrorTypeException, "incomplete msgpack string");
  } else if (result == MPACK_ERROR) {
    api_set_error(err, kErrorTypeException, "invalid msgpack string");
  } else if (result == MPACK_OK && size) {
    api_set_error(err, kErrorTypeException, "trailing data in msgpack string");
  }

  return rv;
}

static void api_parse_enter(mpack_parser_t *parser, mpack_node_t *node)
{
  Unpacker *unpacker = parser->data.p;
  Object *result = unpacker->result;

  switch (node->tok.type) {
    case MPACK_TOKEN_NIL:
      *result = NIL;
      break;
    case MPACK_TOKEN_BOOLEAN:
      *result = BOOL(mpack_unpack_boolean(node->tok));
      break;
    case MPACK_TOKEN_SINT:
      *result = INTEGER_OBJ(mpack_unpack_sint(node->tok));
      break;
    case MPACK_TOKEN_UINT:
      *result = INTEGER_OBJ((Integer)mpack_unpack_uint(node->tok));
      break;
    default:
      *result = INTEGER_OBJ(-1337);
  }
}

static void api_parse_exit(mpack_parser_t *parser, mpack_node_t *node)
{
  Unpacker *unpacker = parser->data.p;
  mpack_node_t *parent = MPACK_PARENT_NODE(node);
}
