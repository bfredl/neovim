
local nvimsrcdir = arg[1]
local shared_file = arg[2]
local funcs_file = arg[3]

_G.vim = loadfile(shared_file)()

if nvimsrcdir == '--help' then
  print([[
Usage:
  lua gen_keyset.lua TODOFIXUPDATETHIS

Will generate build/src/nvim/auto/keyset.generated.h with definition of functions
static const array.
]])
  os.exit(0)
end

package.path = nvimsrcdir .. '/?.lua;' .. package.path
local hashy = require'generators.hashy'

local funcspipe = io.open(funcs_file, 'wb')

local keysets = require'api.keyset'

local name, keys = next(keysets)
neworder, hashfun = hashy.hashy_hash(name, keys, function (idx)
  return name.."_table["..idx.."].str"
end)


funcspipe:write("typedef struct {\n")
for _, key in ipairs(neworder) do
  funcspipe:write("  Object "..key..";\n")
end
funcspipe:write("} KeyDict_"..name..";\n\n")

funcspipe:write("static KeySetLink "..name.."_table[] = {\n")
for _, key in ipairs(neworder) do
  funcspipe:write('  {"'..key..'", offsetof(KeyDict_'..name..", "..key..")},\n")
end
funcspipe:write("};\n\n")

funcspipe:write("static inline "..hashfun)

funcspipe:close()
