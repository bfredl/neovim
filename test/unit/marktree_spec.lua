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

local function pos_leq(a, b)
  return a[1] < b[1] or (a[1] == b[1] and a[2] <= b[2])
end

-- Checks that shadow and tree is consistent, and optionally
-- return the order
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
    local mark = lib.marktree_itr_current(iter)
    local id = tonumber(mark.id)
    local spos = shadow[id]
    if (mark.pos.row ~= spos[1] or mark.pos.col ~= spos[2]) then
      error("invalid pos for "..id..":("..mark.pos.row..", "..mark.pos.col..") instead of ("..spos[1]..", "..spos[2]..")")
    end
    if lib.mt_right_test(mark) ~= spos[3] then
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
  for _, pos in pairs(shadow) do
    if pos_leq(start, pos) then
      if pos_leq(pos, old_end) then
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
  end
end

local function dosplice(tree, shadow, start, old_extent, new_extent)
  lib.marktree_splice(tree, start[1], start[2], old_extent[1], old_extent[2], new_extent[1], new_extent[2])
  shadowsplice(shadow, start, old_extent, new_extent)
end

local ns = 10
local last_id = nil

local function put(tree, row, col, gravitate, end_row, end_col, end_gravitate)
  last_id = last_id + 1
  local my_id = last_id

  end_row = end_row or -1
  end_col = end_col or -1
  end_gravitate = end_gravitate or false

  lib.marktree_put_test(tree, ns, my_id, row, col, gravitate, end_row, end_col, end_gravitate);
  return my_id
end

describe('marktree', function()
  before_each(function()
    last_id = 0
  end)

  itp('works', function()
    local tree = ffi.new("MarkTree[1]") -- zero initialized by luajit
    local shadow = {}
    local iter = ffi.new("MarkTreeIter[1]")
    local iter2 = ffi.new("MarkTreeIter[1]")

    for i = 1,100 do
      for j = 1,100 do
        local gravitate = (i%2) > 0
        local id = put(tree, j, i, gravitate)
        ok(id > 0)
        eq(nil, shadow[id])
        shadow[id] = {j,i,gravitate}
      end
      -- checking every insert is too slow, but this is ok
      lib.marktree_check(tree)
    end

    -- ss = lib.mt_inspect_rec(tree)
    -- io.stdout:write(ffi.string(ss))
    -- io.stdout:flush()

    local id2pos, pos2id = shadoworder(tree, shadow, iter)
    eq({}, pos2id) -- not set if not requested
    eq({}, id2pos)

    for i,ipos in pairs(shadow) do
      local p = lib.marktree_lookup_ns(tree, ns, i, false, iter)
      eq(ipos[1], p.pos.row)
      eq(ipos[2], p.pos.col)
      local k = lib.marktree_itr_current(iter)
      eq(ipos[1], k.pos.row)
      eq(ipos[2], k.pos.col, ipos[1])
      lib.marktree_itr_next(tree, iter)
      -- TODO(bfredl): use id2pos to check neighbour?
      -- local k2 = lib.marktree_itr_current(iter)
    end

    for i,ipos in pairs(shadow) do
      lib.marktree_itr_get(tree, ipos[1], ipos[2], iter)
      local k = lib.marktree_itr_current(iter)
      eq(i, tonumber(k.id))
      eq(ipos[1], k.pos.row)
      eq(ipos[2], k.pos.col)
    end

    ok(lib.marktree_itr_first(tree, iter))
    local del = lib.marktree_itr_current(iter)

    lib.marktree_del_itr(tree, iter, false)
    shadow[tonumber(del.id)] = nil
    shadoworder(tree, shadow, iter)

    for _, ci in ipairs({0,-1,1,-2,2,-10,10}) do
      for i = 1,100 do
        lib.marktree_itr_get(tree, i, 50+ci, iter)
        local k = lib.marktree_itr_current(iter)
        local id = tonumber(k.id)
        eq(shadow[id][1], k.pos.row)
        eq(shadow[id][2], k.pos.col)
        lib.marktree_del_itr(tree, iter, false)
        shadow[id] = nil
      end
      lib.marktree_check(tree)
      shadoworder(tree, shadow, iter)
    end

    -- NB: this is quite rudimentary. We rely on
    -- functional tests exercising splicing quite a bit
    lib.marktree_check(tree)
    dosplice(tree, shadow, {2,2}, {0,5}, {1, 2})
    lib.marktree_check(tree)
    shadoworder(tree, shadow, iter)
    dosplice(tree, shadow, {30,2}, {30,5}, {1, 2})
    lib.marktree_check(tree)
    shadoworder(tree, shadow, iter)

    dosplice(tree, shadow, {5,3}, {0,2}, {0, 5})
    shadoworder(tree, shadow, iter)
    lib.marktree_check(tree)

    -- build then burn (HOORAY! HOORAY!)
    while next(shadow) do
      lib.marktree_itr_first(tree, iter)
      -- delete every other key for fun and profit
      while true do
        local k = lib.marktree_itr_current(iter)
        lib.marktree_del_itr(tree, iter, false)
        ok(shadow[tonumber(k.id)] ~= nil)
        shadow[tonumber(k.id)] = nil
        local stat = lib.marktree_itr_next(tree, iter)
        if not stat then
          break
        end
      end
      lib.marktree_check(tree)
      shadoworder(tree, shadow, iter2)
    end

    -- Check iterator validity for 2 specific edge cases:
    -- https://github.com/neovim/neovim/pull/14719
    lib.marktree_clear(tree)
    for i = 1,20 do
      put(tree, i, i, false)
    end

    lib.marktree_itr_get(tree, 10, 10, iter)
    lib.marktree_del_itr(tree, iter, false)
    eq(11, iter[0].x.key[iter[0].i].pos.col)

    lib.marktree_itr_get(tree, 11, 11, iter)
    lib.marktree_del_itr(tree, iter, false)
    eq(12, iter[0].x.key[iter[0].i].pos.col)
  end)

  itp('works with intersections', function()
    io.stdout:write("\n")
    io.stdout:flush()

    local tree = ffi.new("MarkTree[1]") -- zero initialized by luajit
    local shadow = {}
    local iter = ffi.new("MarkTreeIter[1]")

    function check_intersections()
      if false then
        ok(lib.marktree_check_intersections(tree))
        return
      end
      local str1 = lib.mt_inspect(tree, true, true)
      local dot1 = ffi.string(str1.data, str1.size)

      local val = lib.marktree_check_intersections(tree)
      if not val then
        local str2 = lib.mt_inspect(tree, true, true)
        local dot2 = ffi.string(str2.data, str2.size)
        print("actual:\n\n".."Xafile.dot".."\n\nexpected:\n\n".."Xefile.dot".."\n")
        io.stdout:flush()
        local afil = io.open("Xafile.dot", "wb")
        afil:write(dot1)
        afil:close()
        local efil = io.open("Xefile.dot", "wb")
        efil:write(dot2)
        efil:close()
        ok(false)
      else
        ffi.C.xfree(str1.data)
      end
    end

    local ids = {}

    for i = 1,80 do
      table.insert(ids, put(tree, 1, i, false, 2, 100-i, false))
      check_intersections()
    end
    for i = 1,80 do
      lib.marktree_del_pair_test(tree, ns, ids[i])
      check_intersections()
    end
    ids = {}

    for i = 1,80 do
      table.insert(ids, put(tree, 1, i, false, 2, 100-i, false))
      check_intersections()
    end

    for i = 1,10 do
      for j = 1,8 do
        local ival = (j-1)*10+i
        print("IVAR",  ival, ids[ival])io.stdout:flush()
        lib.marktree_del_pair_test(tree, ns, ids[ival])
        check_intersections()
      end
    end
    ids = {}

    io.stdout:write("GOOFBALL\n")
    io.stdout:flush()
  end)
end)
