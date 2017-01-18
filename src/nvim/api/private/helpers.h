#ifndef NVIM_API_PRIVATE_HELPERS_H
#define NVIM_API_PRIVATE_HELPERS_H

#include <stdbool.h>

#include "nvim/api/private/defs.h"
#include "nvim/vim.h"
#include "nvim/memory.h"
#include "nvim/ex_eval.h"
#include "nvim/lib/kvec.h"

#define OBJECT_OBJ(o) o

#define BOOLEAN_OBJ(b) ((Object) { \
    .type = kObjectTypeBoolean, \
    .data.boolean = b })

#define INTEGER_OBJ(i) ((Object) { \
    .type = kObjectTypeInteger, \
    .data.integer = i })

#define FLOAT_OBJ(f) ((Object) { \
    .type = kObjectTypeFloat, \
    .data.floating = f })

#define STRING_OBJ(s) ((Object) { \
    .type = kObjectTypeString, \
    .data.string = s })

#define BUFFER_OBJ(s) ((Object) { \
    .type = kObjectTypeBuffer, \
    .data.integer = s })

#define WINDOW_OBJ(s) ((Object) { \
    .type = kObjectTypeWindow, \
    .data.integer = s })

#define TABPAGE_OBJ(s) ((Object) { \
    .type = kObjectTypeTabpage, \
    .data.integer = s })

#define ARRAY_OBJ(a) ((Object) { \
    .type = kObjectTypeArray, \
    .data.array = a })

#define DICTIONARY_OBJ(d) ((Object) { \
    .type = kObjectTypeDictionary, \
    .data.dictionary = d })

#define NIL ((Object) {.type = kObjectTypeNil})

#define PUT(dict, k, v) \
  kv_push(dict, ((KeyValuePair) { .key = cstr_to_string(k), .value = v }))

#define ADD(array, item) \
  kv_push(array, item)

#define STATIC_CSTR_AS_STRING(s) ((String) {.data = s, .size = sizeof(s) - 1})

/// Create a new String instance, putting data in allocated memory
///
/// @param[in]  s  String to work with. Must be a string literal.
#define STATIC_CSTR_TO_STRING(s) ((String){ \
    .data = xmemdupz(s, sizeof(s) - 1), \
    .size = sizeof(s) - 1 })

// Helpers used by the generated msgpack-rpc api wrappers
#define api_init_boolean
#define api_init_integer
#define api_init_float
#define api_init_string = STRING_INIT
#define api_init_buffer
#define api_init_window
#define api_init_tabpage
#define api_init_object = NIL
#define api_init_array = ARRAY_DICT_INIT
#define api_init_dictionary = ARRAY_DICT_INIT

#define api_free_boolean(value)
#define api_free_integer(value)
#define api_free_float(value)
#define api_free_buffer(value)
#define api_free_window(value)
#define api_free_tabpage(value)

/// Structure used for saving state for :try
///
/// Used when caller is supposed to be operating when other VimL code is being
/// processed and that “other VimL code” must not be affected.
typedef struct {
  except_T *current_exception;
  struct msglist *private_msg_list;
  const struct msglist *const *msg_list;
  int trylevel;
  int got_int;
  int need_rethrow;
  int did_emsg;
} TryState;

//TODO(timeyyy): this definitoina needs to come from and be used
// by extmarks
#define Extremity -1

// Extmarks may be queried from position or name or even special names
// in the future such as "cursor". This macro sets the line and col
// to make the extmark functions recognize what's required
//
// Relies on unhygenic behavior, following must be defined:
//     buffer
//     namespace
//     err
//
// line: linenr_T, line to be set
// col: colnr_T, col to be set
// extremity: bool, if it's allowed to be an extremity index
#define SET_EXTMARK_INDEX_FROM_OBJ(obj, _line, _col, _set, extremity) { \
  /* Check if it is an extremity */ \
  if (extremity && extmark_is_range_extremity(obj)) { \
    _set = true;\
    _line = Extremity;\
    _col = Extremity;\
  /* Check if it is a mark */ \
  } else if (!_set) { \
    ExtendedMark *_extmark = extmark_from_id_or_pos(buffer,\
                                                    namespace,\
                                                    obj,\
                                                    err,\
                                                    false);\
    if (_extmark) { \
      _set = true;\
      _line = _extmark->line->lnum;\
      _col = _extmark->col;\
    } \
  } \
  /* Check if it is a position */ \
  if (!_set && extmark_is_valid_pos(obj)) { \
    if (obj.type == kObjectTypeArray) { \
      if (obj.data.array.size != 2) { \
        api_set_error(err, Validation, _("Position must have 2 elements"));\
      } else { \
        _set = true;\
        _line = (linenr_T)obj.data.array.items[0].data.integer;\
        _col = (colnr_T)obj.data.array.items[1].data.integer;\
      }\
    } else { \
      api_set_error(err, Validation, _("Position must be in a list"));\
    }\
  } \
}


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "api/private/helpers.h.generated.h"
#endif
#endif  // NVIM_API_PRIVATE_HELPERS_H
