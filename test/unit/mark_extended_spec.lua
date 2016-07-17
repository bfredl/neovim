local helpers = require("test.unit.helpers")
local cimport = helpers.cimport
local ffi = helpers.ffi
local eq = helpers.eq

local mark_extended = cimport("./src/nvim/mark_extended.h")
local pos_cmp = mark_extended.pos_cmp

describe('pos_cmp works like strcmp', function()
  local pos_a = ffi.new('pos_T', {lnum=2, col=2})
  local pos_b = ffi.new('pos_T', {lnum=3, col=3})
  it('Recognizes a less than b', function()
    eq(-1, pos_cmp(pos_a, pos_b))
    pos_b.lnum = pos_a.lnum
    eq(-1, pos_cmp(pos_a, pos_b))
  end)
  it ('Recognizes a is equal to b', function()
    pos_b.lnum = pos_a.lnum
    pos_b.col = pos_a.col
    eq(0, pos_cmp(pos_a, pos_b))
  end)
  it ('Recognizes a is greater than b', function()
    pos_a.col = pos_b.col + 1
    eq(1, pos_cmp(pos_a, pos_b))
    pos_a.col = pos_b.col
    pos_a.lnum = pos_b.lnum + 1
    eq(1, pos_cmp(pos_a, pos_b))
  end)
end)
