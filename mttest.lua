ffi = require'ffi'
cdefs = io.open('build/cdefs.dump.h', 'r'):read('*all')
string.len(cdefs)
ffi.cdef(cdefs)

function mtpos(row,col)
  local setpos = ffi.new("mtpos_t")
  setpos.row = row
  setpos.col = col
  return setpos
end

p = require'luadev'.print
tree = ffi.C.marktree_new()
iter = ffi.new("MarkTreeIter[1]")
dibbl = {}


for i = 1,20 do
  for j = 1,20 do
    id = ffi.C.marktree_put(tree, i, j,j%2)
    if dibbl[id] then error("DIBBL!") end
    dibbl[id] = {j,i}
  end
end
if false then
idx = ffi.C.marktree_put(tree, 2, 2,true)
idy = ffi.C.marktree_put(tree, 2, 2,false)
idz = ffi.C.marktree_put(tree, 2, 2,true)
end
aaa()


ffi.C.marktree_splice(tree, mtpos(2,5), mtpos(0,3), mtpos(0,3), iter)
ss = ffi.C.mt_inspect_rec(tree) p(ffi.string(ss))

--ffi.C.marktree_itr_get(tree, key, iter)
ffi.C.marktree_itr_first(tree, iter)
i = 1
repeat
  val = ffi.C.marktree_itr_test(iter)
  p(val.pos.row, val.pos.col, val.id)
  pos2 = dibbl[tonumber(val.id)]
  --p(ffi.C.marktree_itr_next(tree, iter))
  --iter[0].lvl
  --iter[0].node[0]
  --iter[0].lvl
  if val.pos.row > 3 then
    error("x")
  end
  i = i +1
until not ffi.C.marktree_itr_next(tree, iter) -- i == 109 --

aaa()
if false then
  setpos = ffi.new("mtpos_t")
  setpos.row = 2
  setpos.col = 10
  -- TODO: kolla lookup!
  ffi.C.marktree_itr_get(tree, setpos, iter)
  ffi.C.marktree_del_itr(tree, iter, false)

end

local status = ffi.C.marktree_itr_first(tree, iter)
ffi.C.marktree_del_itr(tree, iter, false)
raa()

for i,ipos in pairs(dibbl) do
  pos = ffi.C.marktree_lookup(tree, i, iter)
  if pos.row ~= ipos[1] or pos.col ~= ipos[2] then
    error("eee "..i)
  end
  k = ffi.C.marktree_itr_test(iter)
  if k.pos.row ~= ipos[1] or k.pos.col ~= ipos[2] then
    error("itr eee "..i)
  end
  ffi.C.marktree_itr_next(tree, iter)
  k = ffi.C.marktree_itr_test(iter)
  p(vim.inspect(ipos), vim.inspect(k2.pos))
end

p(pos.row, pos.col)


--ffi.C.marktree_itr_get(tree, key, iter)
ffi.C.marktree_itr_first(tree, iter)
i = 1
repeat
  val = ffi.C.marktree_itr_test(iter)
  p(val.pos.row, val.pos.col, val.id)
  pos2 = dibbl[tonumber(val.id)]
  --p(ffi.C.marktree_itr_next(tree, iter))
  --iter[0].lvl
  --iter[0].node[0]
  --iter[0].lvl

  if val.pos.row > 3 then
    error("x")
  end
  i = i +1
until not ffi.C.marktree_itr_next(tree, iter) -- i == 109 --

ee()

--val.id

