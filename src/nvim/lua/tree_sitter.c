// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// lua bindings for tree-siter.
// NB: this file should contain a generic lua interface for
// tree-sitter trees and nodes, and could be broken out as a reusable library

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <tree_sitter/runtime.h>

#define REG_KEY "tree_sitter-private"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "lua/tree_sitter.c.generated.h"
#endif

static struct luaL_Reg tree_meta[] = {
  // {"__gc", tree_gc},
  {"root", tree_root},
  {NULL, NULL}
};

static struct luaL_Reg node_meta[] = {
  {NULL, NULL}
};

void build_meta(lua_State *L, const luaL_Reg *meta)
{
  // [env, target]
  for (size_t i = 0; meta[i].name != NULL; i++) {
    lua_pushcfunction(L, meta[i].func);  // [env, target, func]
    lua_pushvalue(L, -3);  // [env, target, func, env]
    lua_setfenv(L, -2);  // [env, target, func]
    lua_setfield(L, -2, meta[i].name);  // [env, target]
  }

  lua_pushvalue(L, -1);  // [env, target, target]
  lua_setfield(L, -2, "__index");  // [env, target]
}



/// init the tslua library
///
/// all global state is stored in the regirstry of the lua_State
void tslua_init(lua_State *L)
{
  lua_createtable(L, 0, 0);

  // Tree metatable
  lua_createtable(L, 0, 0);
  build_meta(L, tree_meta);
  lua_setfield(L, -2, "tree-meta");

  lua_createtable(L, 0, 0);
  build_meta(L, node_meta);
  lua_setfield(L, -2, "node-meta");

  lua_setfield(L, LUA_REGISTRYINDEX, REG_KEY);
}

/// push tree interface on lua stack.
///
/// This takes "ownership" of the tree and will free it
/// when the wrapper object is garbage collected
void tslua_push_tree(lua_State *L, TSTree *tree)
{
  TSTree **ud = lua_newuserdata(L, sizeof(TSTree *));  // [udata]
  *ud = tree;
  lua_getfield(L, LUA_REGISTRYINDEX, REG_KEY);  // [udata, env]
  lua_getfield(L, -1, "tree-meta");  // [udata, env, meta]
  lua_setmetatable(L, -3);  // [udata, env]
  lua_pop(L, 1);  // [udata]

  // table used for node wrappers to keep a reference to tree wrapper
  // NB: in lua 5.3 the uservalue for the node could just be the tree, but
  // in lua 5.1 the uservalue (fenv) must be a table.
  lua_createtable(L, 1, 0); // [udata, reftable]
  lua_pushvalue(L, -2); // [udata, reftable, udata]
  lua_rawseti(L, -2, 1); // [udata, reftable]
  lua_setfenv(L, -2); // [udata]
}

static TSTree *tree_check(lua_State *L)
{
  if (!lua_gettop(L)) {
    return 0;
  }
  if (!lua_isuserdata(L, 1)) {
    return 0;
  }
  TSTree **ud = lua_touserdata(L, 1);
  return *ud;
}


static int tree_root(lua_State *L)
{
  TSTree *tree = tree_check(L);
  if (!tree) {
    return 0;
  }
  TSNode root = ts_tree_root_node(tree);
  push_node(L, root);
  return 1;
}


/// push node interface on lua stack
///
/// top of stack must either be the tree this node belongs to or another node
/// of the same tree! This value is not popped. Can only be called inside a
/// cfunction with the tslua environment.
static void push_node(lua_State *L, TSNode node)
{
  TSNode *ud = lua_newuserdata(L, sizeof(TSNode));  // [src, udata]
  *ud = node;
  lua_getfield(L, LUA_ENVIRONINDEX, "node-meta");  // [src, udata, meta]
  lua_setmetatable(L, -2);  // [src, udata]
  lua_getfenv(L, -2);  // [src, udata, reftable]
  lua_setfenv(L, -2);  // [src, udata]
}

