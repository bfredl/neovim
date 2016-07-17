
local helpers = require('test.functional.helpers')(after_each)

local eq = helpers.eq
local neq = helpers.neq
local buffer = helpers.buffer
local insert = helpers.insert
local wait = helpers.wait

describe('creating a new mark', function()
  helpers.clear()

  it('works', function()
    local buf = helpers.nvim('get_current_buffer')
    local string_mark = "test"
    local text = "some text to insert"
    local row = 0
    local col = 0

    local rv = buffer('mark_set', buf, string_mark, row, col)
    wait()
    eq(1, rv)
    local pos = buffer('mark_index', buf, string_mark)
    neq(pos, nil)
    eq(row, pos[1])
    eq(col, pos[2])
    -- Move the mark
    -- insert(text)
    -- wait()
    -- pos = buffer('mark_index', buf, string_mark)
    -- neq(pos, nil)
    -- eq(row, pos[1])
    -- eq(string.len(text), pos[2])

    -- updating an existing mark
    -- row = 0
    -- col = 0
    -- local rv = buffer('mark_set', buf, string_mark, row, col)
    -- wait()
    -- eq(2, rv)
    -- local pos = buffer('mark_index', buf, string_mark)
    -- eq(row, pos[1])
    -- eq(text.length(), pos[2])
  end)
end)

