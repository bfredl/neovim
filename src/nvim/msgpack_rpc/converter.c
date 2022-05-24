// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "nvim/api/private/helpers.h"
#include "nvim/msgpack_rpc/converter.h"
#include "nvim/log.h"
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
  NVIM_PROBE(start_convert, 2, data, size);

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
  NVIM_PROBE(parse_enter, 2, node->tok.type, node->tok.length);
  Object *result;
  String *key_location = NULL;

  mpack_node_t *parent = MPACK_PARENT_NODE(node);
  if (parent) {
    switch (parent->tok.type) {
      case MPACK_TOKEN_ARRAY: {
        Object *obj = parent->data[0].p;
        NVIM_PROBE(parse_array, 2, obj->data.array.capacity, parent->pos);
        result = &kv_A(obj->data.array, parent->pos);
        break;
      }
      case MPACK_TOKEN_MAP: {
        Object *obj = parent->data[0].p;
        NVIM_PROBE(parse_dict, 2, obj->data.dictionary.capacity, parent->pos);
        KeyValuePair *kv = &kv_A(obj->data.dictionary, parent->pos);
        if (parent->key_visited) {
          key_location = &kv->key;
        } else {
          result = &kv_A(obj->data.array, parent->pos);
        }
        break;
      }

      default:
        break;
    }
  } else {
    result = unpacker->result;
  }

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
    case MPACK_TOKEN_FLOAT:
      *result = FLOAT_OBJ(mpack_unpack_float(node->tok));
      break;
    case MPACK_TOKEN_BIN:
    case MPACK_TOKEN_STR: {
      String str = {.data = xmallocz(node->tok.length), .size = node->tok.length};

      if (key_location) {
        *key_location = str;
      } else {
        *result = STRING_OBJ(str);
      }

      node->data[0].p = str.data;
      break;
    }
    case MPACK_TOKEN_CHUNK: {
      char *data = parent->data[0].p;
      memcpy(data + parent->pos,
             node->tok.data.chunk_ptr, node->tok.length);
      break;
    }
    case MPACK_TOKEN_ARRAY: {
      Array arr = KV_INITIAL_VALUE;
      kv_resize(arr, node->tok.length);
      kv_size(arr) = node->tok.length;
      *result = ARRAY_OBJ(arr);
      node->data[0].p = result;
      break;
    }
    case MPACK_TOKEN_MAP: {
      Dictionary dict = KV_INITIAL_VALUE;
      kv_resize(dict, node->tok.length);
      kv_size(dict) = node->tok.length;
      *result = DICTIONARY_OBJ(dict);
      node->data[0].p = result;
      // unpacker->result = &kv_A(arr, 0);
      break;
    }
    default:
      *result = INTEGER_OBJ(-1337);
  }
}

static void api_parse_exit(mpack_parser_t *parser, mpack_node_t *node)
{
  NVIM_PROBE(parse_exit, 2, node->tok.type, node->tok.length);
}
