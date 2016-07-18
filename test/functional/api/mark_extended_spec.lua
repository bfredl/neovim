
local helpers = require('test.functional.helpers')(after_each)

local eq = helpers.eq
local neq = helpers.neq
local buffer = helpers.buffer
local insert = helpers.insert
local feed = helpers.feed
local wait = helpers.wait

describe('creating a new mark', function()
  helpers.clear()

  it('works', function()
    local buf = helpers.nvim('get_current_buffer')
    local string_mark = "test"
    local row = 0
    local col = 2

    -- mark the "3"
    local init_text = "123"
    insert(init_text)
    local rv = buffer('mark_set', buf, string_mark, row, col)
    wait()
    eq(1, rv)
    local pos = buffer('mark_index', buf, string_mark)
    neq(pos, nil)
    eq(row, pos[1])
    eq(col, pos[2])

    -- marks move with text inserts?
    added_text = "456"
    insert(added_text)
    -- after inserting text we cannot get our mark anymore..
    pos = buffer('mark_test', buf, string_mark)
    -- above throws error
    neq(pos, nil)
    eq(row, pos[1])
    eq(string.len(init_text + added_text), pos[2])

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

