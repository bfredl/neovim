local helpers = require("test.unit.helpers")(after_each)
local itp = helpers.gen_itp(it)

local ffi     = helpers.ffi
local eq      = helpers.eq
local ok      = helpers.ok

local lib = helpers.cimport("./src/nvim/marktree.h")

local function tablelength(t)
  local count = 0
  for _ in pairs(t) do count = count + 1 end
  return count
end

local function shadoworder(tree, shadow, iter)
  ok(iter ~= nil)
  local status = lib.marktree_itr_first(tree, iter)
  local count = 0
  local pos2id, id2pos = {}, {}
  local last
  if not status and next(shadow) == nil then
    return pos2id, id2pos
  end
  repeat
    local key = lib.marktree_itr_test(iter)
    local id = tonumber(key.id)
    local spos = shadow[id]
    if (key.pos.row ~= spos[1] or key.pos.col ~= spos[2]) then
      error("invalid pos for "..id..": ("..spos[1]..", "..spos[2]..")")
    end
    if count > 0 then
      if spos[1] < last[1] or (spos[1] == last[1] and spos[2] < last[2]) then
        error("DISORDER")
      end
    end
    count = count + 1
    last = spos
    pos2id[count] = id
    id2pos[id] = count
  until not lib.marktree_itr_next(tree, iter)
  if tablelength(shadow) ~= count then
    error("missed some keys?")
  end
  return id2pos, pos2id
end

describe('marktree', function()
 itp('works', function()
    tree = lib.marktree_new(1)
    local shadow = {}
    iter = ffi.new("MarkTreeIter[1]")

    for i = 1,100 do
      for j = 1,100 do
        id = tonumber(lib.marktree_put(tree, j, i))
        ok(id > 0)
        eq(nil, shadow[id])
        shadow[id] = {j,i}
      end
      -- checking every insert is too slow, but this is ok
      --lib.marktree_check(tree)
    end

    if false then
      ss = lib.mt_inspect_rec(tree)
      io.stdout:write(ffi.string(ss))
      io.stdout:flush()
    end

    local id2pos, pos2id = shadoworder(tree, shadow, iter)
    for i,ipos in pairs(shadow) do
      local pos = lib.marktree_lookup(tree, i, iter)
      eq(ipos[1], pos.row)
      eq(ipos[2], pos.col)
      local k = lib.marktree_itr_test(iter)
      eq(ipos[1], k.pos.row)
      eq(ipos[2], k.pos.col, ipos[1])
      lib.marktree_itr_next(tree, iter)
      local k2 = lib.marktree_itr_test(iter)
      -- TODO: use id2pos to chechk neighbour
    end

    local setpos = ffi.new("mtpos_t")
    for i,ipos in pairs(shadow) do
      setpos.row = ipos[1]
      setpos.col = ipos[2]
      lib.marktree_itr_get(tree, setpos, iter)
      local k = lib.marktree_itr_test(iter)
      eq(i, tonumber(k.id))
      eq(ipos[1], k.pos.row)
      eq(ipos[2], k.pos.col)
    end

    local status = lib.marktree_itr_first(tree, iter)
    lib.marktree_check(tree)
    lib.marktree_del_itr(tree, iter, false)
    lib.marktree_check(tree)

    for i = 1,100 do
      setpos.row = i
      setpos.col = 50
      -- TODO: kolla lookup!
      lib.marktree_itr_get(tree, setpos, iter)
      lib.marktree_del_itr(tree, iter, false)
      lib.marktree_check(tree)
    end

    if true then
      -- TODO: remove this one crash testing is fixed
      io.stdout:write('\nDUN\n')
      io.stdout:flush()
      return
    end
 end)
end)
