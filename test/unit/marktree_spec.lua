local helpers = require("test.unit.helpers")(after_each)
local itp = helpers.gen_itp(it)

local ffi     = helpers.ffi
local eq      = helpers.eq
local neq     = helpers.neq
local ok      = helpers.ok

local lib = helpers.cimport("./src/nvim/marktree.h")

local inspect = require'vim.inspect'

local function tablelength(t)
  local count = 0
  for _ in pairs(t) do count = count + 1 end
  return count
end

local function pos_leq(a, b)
  return a[1] < b[1] or (a[1] == b[1] and a[2] <= b[2])
end

local function shadoworder(tree, shadow, iter, giveorder)
  ok(iter ~= nil)
  local status = lib.marktree_itr_first(tree, iter)
  local count = 0
  local pos2id, id2pos = {}, {}
  local last
  if not status and next(shadow) == nil then
    return pos2id, id2pos
  end
  repeat
    local mark = lib.marktree_itr_test(iter)
    local id = tonumber(mark.id)
    local spos = shadow[id]
    if (mark.pos.row ~= spos[1] or mark.pos.col ~= spos[2]) then
      error("invalid pos for "..id..":("..mark.pos.row..", "..mark.pos.col..") instead of ("..spos[1]..", "..spos[2]..")")
    end
    if mark.right_gravity ~= spos[3] then
        error("invalid gravity for "..id..":("..mark.pos.row..", "..mark.pos.col..")")
    end
    if count > 0 then
      if not pos_leq(last, spos) then
        error("DISORDER")
      end
    end
    count = count + 1
    last = spos
    if giveorder then
      pos2id[count] = id
      id2pos[id] = count
    end
  until not lib.marktree_itr_next(tree, iter)
  local shadowlen = tablelength(shadow)
  if shadowlen ~= count then
    error("missed some keys? (shadow "..shadowlen..", tree "..count..")")
  end
  return id2pos, pos2id
end

local function shadowsplice(shadow, start, old_extent, new_extent)
  local old_end = {start[1] + old_extent[1],
                      (old_extent[1] == 0 and start[2] or 0) + old_extent[2]}
  local new_end = {start[1] + new_extent[1],
                      (new_extent[1] == 0 and start[2] or 0) + new_extent[2]}
  local delta = {new_end[1] - old_end[1], new_end[2] - old_end[2]}
  for iid, pos in pairs(shadow) do
    if not pos_leq(start, pos) then
      -- do nothing
    elseif pos_leq(pos, old_end) then
      -- delete region
      if pos[3] then -- right gravity
        pos[1], pos[2] = new_end[1], new_end[2]
      else
        pos[1], pos[2] = start[1], start[2]
      end
    else
      if pos[1] == old_end[1] then
        pos[2] = pos[2] + delta[2]
      end
      pos[1] = pos[1] + delta[1]
    end
  end
      io.stdout:flush()
end

local function mtpos(row,col)
  local setpos = ffi.new("mtpos_t")
  setpos.row = row
  setpos.col = col
  return setpos
end

local function mtpos2(pos)
  return mtpos(unpack(pos))
end

local function dosplice(tree, shadow, start, old_extent, new_extent)
  lib.marktree_splice(tree, mtpos2(start), mtpos2(old_extent), mtpos2(new_extent))
  shadowsplice(shadow, start, old_extent, new_extent)
end

describe('marktree', function()
 itp('works', function()
    tree = lib.marktree_new()
    local shadow = {}
    iter = ffi.new("MarkTreeIter[1]")

    for i = 1,100 do
      for j = 1,100 do
        local gravitate = (i%2) > 0
        id = tonumber(lib.marktree_put(tree, j, i, gravitate))
        ok(id > 0)
        eq(nil, shadow[id])
        shadow[id] = {j,i,gravitate}
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

    for i,ipos in pairs(shadow) do
      lib.marktree_itr_get(tree, mtpos(unpack(ipos)), iter)
      local k = lib.marktree_itr_test(iter)
      eq(i, tonumber(k.id))
      eq(ipos[1], k.pos.row)
      eq(ipos[2], k.pos.col)
    end

    local status = lib.marktree_itr_first(tree, iter)
    --lib.marktree_check(tree)
    lib.marktree_del_itr(tree, iter, false)
    --lib.marktree_check(tree)
    shadow[1] = nil
    --ss = lib.mt_inspect_rec(tree) print(ffi.string(ss))
    id2pos, pos2id = shadoworder(tree, shadow, iter)

    for _, ci in ipairs({0,-1,1,-2,2,-10,10}) do
      for i = 1,100 do
        -- TODO: kolla lookup!
        lib.marktree_itr_get(tree, mtpos(i,50+ci), iter)
        local k = lib.marktree_itr_test(iter)
        local id = tonumber(k.id)
        eq(shadow[id][1], k.pos.row)
        eq(shadow[id][2], k.pos.col)
        lib.marktree_del_itr(tree, iter, false)
        --lib.marktree_check(tree)
        shadow[id] = nil
        --id2pos, pos2id = shadoworder(tree, shadow, iter)
        -- TODO: update the shadow and check!
      end
      id2pos, pos2id = shadoworder(tree, shadow, iter)
    end
    id2pos, pos2id = shadoworder(tree, shadow, iter)

    --print(inspect(shadow[702])) io.stdout:flush()
    --lib.marktree_check(tree)
    lib.marktree_check(tree)
    dosplice(tree, shadow, {2,2}, {0,5}, {1, 2})
    lib.marktree_check(tree)
    id2pos, pos2id = shadoworder(tree, shadow, iter)
    dosplice(tree, shadow, {30,2}, {30,5}, {1, 2})
    lib.marktree_check(tree)
    id2pos, pos2id = shadoworder(tree, shadow, iter)

    dosplice(tree, shadow, {5,3}, {0,2}, {0, 5})
    id2pos, pos2id = shadoworder(tree, shadow, iter)
    lib.marktree_check(tree)


    if true then
      -- TODO: remove this one crash testing is fixed
      io.stdout:write('\nDUN\n')
      io.stdout:flush()
      return
    end
 end)
end)
