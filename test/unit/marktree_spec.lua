local helpers = require("test.unit.helpers")(after_each)
local itp = helpers.gen_itp(it)

local ffi     = helpers.ffi
local eq      = helpers.eq
local ok      = helpers.ok

local lib = helpers.cimport("./src/nvim/marktree.h")

describe('marktree', function()
 itp('works', function()
    tree = lib.marktree_new(1)
    shadow = {}
    iter = ffi.new("MarkTreeIter[1]")

    for i = 1,100 do
      for j = 1,100 do
        id = lib.marktree_put(tree, j, i)
        ok(id > 0)
        eq(nil, shadow[id])
        shadow[id] = {j,i}
      end
    end

    for i,ipos in pairs(shadow) do
      local pos = lib.marktree_lookup(tree, i, iter)
      eq(ipos[1], pos.row)
      eq(ipos[2], pos.col)
      local k = lib.marktree_itr_test(iter)
      eq(ipos[1], k.pos.row)
      eq(ipos[2], k.pos.col)
      lib.marktree_itr_next(tree, iter)
      -- TODO
      --local k2 = lib.marktree_itr_test(iter)
    end
 end)
end)
