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
  int oldcol;
  int i;
} iterstate_t;

typedef struct {
  mtkey_t pos;
  int lvl;
  mtnode_t *node;
  int i;
  iterstate_t s[64];
} MarkTreeIter;


struct mtnode_s {
  int32_t n;
  bool is_internal;
  mtkey_t key[19];
  mtnode_t *parent;
  mtnode_t *ptr[];
};


MarkTree *marktree_new(bool rel);
void marktree_put(MarkTree *b, mtkey_t k);
void marktree_put_pos(MarkTree *b, int row, int col, uint64_t id);
int marktree_itr_get(MarkTree *b, mtkey_t k, MarkTreeIter *itr);
void marktree_itr_first(MarkTree *b, MarkTreeIter *itr);
bool marktree_itr_next(MarkTree *b, MarkTreeIter *itr);
int marktree_itr_prev(MarkTree *b, MarkTreeIter *itr);
mtkey_t marktree_itr_test(MarkTreeIter *itr);
char *mt_inspect_rec(MarkTree *b);
]])


p = require'luadev'.print
tree = ffi.C.marktree_new(0)
iter = ffi.new("MarkTreeIter[1]")
for i = 1,300 do
  ffi.C.marktree_put_pos(tree, 1, i, i)
end
ss = ffi.C.mt_inspect_rec(tree)
p(ffi.string(ss))

raa()

key = ffi.new("mtkey_t")
key.row = -1
--ffi.C.marktree_itr_get(tree, key, iter)
ffi.C.marktree_itr_first(tree, iter)
i = 1
repeat

  val = ffi.C.marktree_itr_test(iter) p(val.row, val.col, val.id)
  p(ffi.C.marktree_itr_next(tree, iter))
  --iter[0].lvl
  --iter[0].node[0]
  --iter[0].lvl

  if false and val.col ~= i then
    error("x")
  end
  i = i +1
until not ffi.C.marktree_itr_next(tree, iter) -- i == 109 --

ee()

--val.id

