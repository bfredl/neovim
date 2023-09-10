#ifndef NVIM_TYPES_H
#define NVIM_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// dummy to pass an ACL to a function
typedef void *vim_acl_T;

// Can hold one decoded UTF-8 character.
typedef uint32_t u8char_T;

// Opaque handle used by API clients to refer to various objects in vim
typedef int handle_T;

// Opaque handle to a lua value. Must be free with `api_free_luaref` when
// not needed anymore! LUA_NOREF represents missing reference, i e to indicate
// absent callback etc.
typedef int LuaRef;

/// Type used for Vimscript VAR_FLOAT values
typedef double float_T;

typedef struct MsgpackRpcRequestHandler MsgpackRpcRequestHandler;

typedef union {
  float_T (*float_func)(float_T);
  const MsgpackRpcRequestHandler *api_handler;
  void *null;
} EvalFuncData;

typedef handle_T NS;

typedef struct expand expand_T;

typedef uint64_t proftime_T;

typedef enum {
  kNone  = -1,
  kFalse = 0,
  kTrue  = 1,
} TriState;

#define TRISTATE_TO_BOOL(val, \
                         default) ((val) == kTrue ? true : ((val) == kFalse ? false : (default)))

typedef struct Decoration Decoration;

/// A block number.
///
/// Blocks numbered from 0 upwards have been assigned a place in the actual
/// file. The block number is equal to the page number in the file. The blocks
/// with negative numbers are currently in memory only.
typedef int64_t blocknr_T;

/// A block header.
///
/// There is a block header for each previously used block in the memfile.
///
/// The used list is a doubly linked list, most recently used block first.
/// The blocks in the used list have a block of memory allocated.
/// The free list is a single linked list, not sorted.
/// The blocks in the free list have no block of memory allocated and
/// the contents of the block in the file (if any) is irrelevant.
typedef struct bhdr {
  blocknr_T bh_bnum;                 ///< key used in hash table

  void *bh_data;                     ///< pointer to memory (for used block)
  unsigned bh_page_count;            ///< number of pages in this block

  unsigned bh_flags;                 ///< BH_DIRTY or BH_LOCKED
} bhdr_T;


#endif  // NVIM_TYPES_H
