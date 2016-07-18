
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
    local row = 0
    local col = 3

    -- mark the "3"
    insert("123")
    local rv = buffer('mark_set', buf, string_mark, row, col)
    wait()
    eq(1, rv)
    local pos = buffer('mark_index', buf, string_mark)
    neq(pos, nil)
    eq(row, pos[1])
    eq(col, pos[2])

    -- marks move with text inserts?
    -- text = "456"
    -- insert(text)
    -- wait()
    -- pos = buffer('mark_index', buf, string_mark)
    -- neq(pos, nil)
    -- eq(row, pos[1])
    -- eq(string.len(text), pos[2])

    -- Update an existing mark
    row = 0
    col = 5
    rv = buffer('mark_set', buf, string_mark, row, col)
    wait()
    eq(2, rv)
    pos = buffer('mark_index', buf, string_mark)
    eq(row, pos[1])
    eq(col, pos[2])

    -- remove a mark
    rv = buffer('mark_unset', buf, string_mark)
    eq(1, rv)
    -- TODO catch the error..
    -- pos = buffer('mark_index', buf, string_mark)
  end)
end)

