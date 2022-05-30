// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "nvim/api/private/helpers.h"
#include "nvim/msgpack_rpc/converter.h"
#include "nvim/log.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "msgpack_rpc/converter.c.generated.h"
#endif


Object convert(const char *data, size_t size, Error *err)
{
  Unpacker unpacker;
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

  return unpacker.result;
}

static void api_parse_enter(mpack_parser_t *parser, mpack_node_t *node)
{
  Unpacker *unpacker = parser->data.p;
  NVIM_PROBE(parse_enter, 2, node->tok.type, node->tok.length);
  Object *result = NULL;
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
        NVIM_PROBE(parse_dict, 2, parent->pos, parent->key_visited);
        KeyValuePair *kv = &kv_A(obj->data.dictionary, parent->pos);
        if (!parent->key_visited) {
          key_location = &kv->key;
        } else {
          result = &kv->value;
        }
        break;
      }

      default:
        break;
    }
  } else {
    result = &unpacker->result;
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

void unpacker_init(Unpacker *p)
{
  mpack_parser_init(&p->parser, 0); // TODO: fyyy
  p->parser.data.p = p;
  mpack_tokbuf_init(&p->reader); // TODO: fyyy
}

bool unpacker_advance_tok(Unpacker *p, mpack_token_t tok) {
  NVIM_PROBE(toky, 2, p->state, tok.type);
  switch (p->state) {
    case 0:
      if (tok.type != MPACK_TOKEN_ARRAY || tok.length < 3 || tok.length > 4)  {
        abort();
      }
      p->state = tok.length == 3 ? 2 : 1;
      return true;

    case 1:
    case 2:
      if (tok.type != MPACK_TOKEN_UINT || tok.length > 1) abort();
      uint32_t type = tok.data.value.lo;
      if (p->state == 2 ? type != 2 : (type >= 2)) abort();
      p->type = (MessageType)type;
      p->state = 3+(int)type;
      return true;
    
    case 3+0: // REQUEST
      // TODO: om näll request_id > 2^32-1 ?
      if (tok.type != MPACK_TOKEN_UINT || tok.length > 1) abort();
      p->request_id = tok.data.value.lo;
      p->state = 6;
      return true;

    case 3+1:
      if (tok.type != MPACK_TOKEN_UINT || tok.length > 1) abort();
      p->request_id = tok.data.value.lo;
      p->state = 9;
      return true;

    case 3+2: // NOTIFY
      // no id, jump directly to method name
      p->request_id = 0;
      FALLTHROUGH;

    case 6:
      if (tok.type != MPACK_TOKEN_STR && tok.type != MPACK_TOKEN_BIN) abort();
      if (tok.length > 100) abort();
      p->method_name_len = tok.length;
      // Don't use the chunk state of p->reader, here
      // let's manage method name ourselves
      mpack_tokbuf_init(&p->reader);
      p->state = 7;
      return true;

    default:
      abort();
  }
  return false;
}

bool unpacker_advance(Unpacker *p)
{
  const char *data = p->fulbuffer + p->read;
  size_t size = p->written - p->read;

  NVIM_PROBE(advance, 2, p->state, size);

  int result;
  while (p->state < 7 && size) {
    mpack_token_t tok;
    result = mpack_read(&p->reader, &data, &size, &tok);
    if (result) break;
    if (!unpacker_advance_tok(p, tok)) {
      p->read = p->written - size;
      return false;
    }
  }
  p->read = p->written - size;

  if (p->state == 7) {
    if (size < p->method_name_len) {
      p->read = p->written - size;
      return false; // wait for full method name to arrive
    }

    // TODO: a global error object for the unpacker (use everywhere abort() is
    // used...)
    Error err = ERROR_INIT;
    // if this fails, p->handler.fn will be NULL
    p->handler = msgpack_rpc_get_handler_for(data, p->method_name_len, &err);
    p->state = 8;
    data += p->method_name_len;
    size -= p->method_name_len;
  }

rerun:
  result = mpack_parse(&p->parser, &data, &size, api_parse_enter,
      api_parse_exit);

  p->read = p->written - size;

  if (result == MPACK_NOMEM) {
    abort();
  } else if (result == MPACK_EOF) {
    return false;
  } else if (result == MPACK_ERROR) {
    abort();
  }

  assert(result == MPACK_OK);

  switch (p->state) {
    case 8:
      p->state = 0;
      break;
    case 9:
      p->error = p->result;
      p->state = 8;
      goto rerun;
    default:
      abort();
  }

  return true;
}
