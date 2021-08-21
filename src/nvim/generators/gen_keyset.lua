local mpack = require('mpack')

local nvimsrcdir = arg[1]
local funcs_file = arg[2]

if nvimsrcdir == '--help' then
  print([[
Usage:
  lua gen_eval.lua src/nvim build/src/nvim/auto

Will generate build/src/nvim/auto/funcs.generated.h with definition of functions
static const array.
]])
  os.exit(0)
end

package.path = nvimsrcdir .. '/?.lua;' .. package.path

local funcspipe = io.open(funcs_file, 'wb')

local keysets = require'api.keyset'


funcspipe:write((next(keysets)))


funcspipe:close()
