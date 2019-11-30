ffi = require'ffi'
cdefs = io.open('build/cdefs.dump.h', 'r'):read('*all')
string.len(cdefs)
ffi.cdef(cdefs)



p = require'luadev'.print
tree = ffi.C.marktree_new(1)
iter = ffi.new("MarkTreeIter[1]")
dibbl = {}


for i = 1,10 do
  for j = 1,10 do
    id = ffi.C.marktree_put(tree, j, i)
    if dibbl[id] then error("DIBBL!") end
    dibbl[id] = {j,i}
  end
end

ss = ffi.C.mt_inspect_rec(tree)
p(ffi.string(ss))

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
  val = ffi.C.marktree_itr_test(iter) p(val.pos.row, val.pos.col, val.id)
  pos2 = dibbl[tonumber(val.id)]
  --p(ffi.C.marktree_itr_next(tree, iter))
  --iter[0].lvl
  --iter[0].node[0]
  --iter[0].lvl

  if true and (val.pos.row ~= pos2[1] or val.pos.col ~= pos2[2]) then
    error("x")
  end
  i = i +1
until not ffi.C.marktree_itr_next(tree, iter) -- i == 109 --
i

ee()

--val.id

