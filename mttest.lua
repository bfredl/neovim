ffi = require'ffi'
ffi.cdef([[
typedef struct {
  int32_t row;
  int32_t col;
  uint64_t id;
} mtkey_t;

typedef struct mttree_s MarkTree;
typedef struct mtnode_s mtnode_t;

typedef struct {
  // TODO(bfredl): really redundant with "backlinks", but keep now so we can do
  // consistency check easily.
  mtnode_t *x;
  int i;
} mtpos_t;

typedef struct {
  mtkey_t pos;
  mtpos_t stack[64], *p;
} MarkTreeIter;


MarkTree *marktree_new(bool rel);
void marktree_put(MarkTree *b, mtkey_t k);
void marktree_put_pos(MarkTree *b, int row, int col, uint64_t id);
int marktree_itr_get(MarkTree *b, mtkey_t k, MarkTreeIter *itr);
int marktree_itr_next(MarkTree *b, MarkTreeIter *itr);
int marktree_itr_prev(MarkTree *b, MarkTreeIter *itr);
mtkey_t marktree_itr_test(MarkTreeIter *itr);
char *mt_inspect_rec(MarkTree *b);
]])


p = require'luadev'.print
tree = ffi.C.marktree_new(1)
iter = ffi.new("MarkTreeIter[1]")
for i = 1,400 do
  ffi.C.marktree_put_pos(tree, 1, i, i)
end
ss = ffi.C.mt_inspect_rec(tree)
p(ffi.string(ss))

raa()

key = ffi.new("mtkey_t")
key.row = -1
ffi.C.marktree_itr_get(tree, key, iter)
i = 1
while ffi.C.marktree_itr_next(tree, iter) > 0 do
  val = ffi.C.marktree_itr_test(iter)
  p(val.row, val.col, val.id)
  if false and val.col ~= i then
    error("x")
  end
  i = i +1
end

--val.id

