
local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')

local eq = helpers.eq
local neq = helpers.neq
local buffer = helpers.buffer
local insert = helpers.insert
local feed = helpers.feed
local wait = helpers.wait

describe('creating a new mark', function()
  local screen
  helpers.clear()
  helpers.nvim('set_option', 'shell', helpers.nvim_dir .. '/shell-test')
  helpers.nvim('set_option', 'shellcmdflag', 'EXE')
  screen = Screen.new(20, 4)
  screen:attach(false)

  it('works', function()
    local buf = helpers.nvim('get_current_buffer')
    local string_mark = "test"
    local ns = "test-namespace"
    local row = 0
    local col = 2

    local init_text = "12345"
    insert(init_text)
    -- mark the "2"
    local rv = buffer('mark_set', buf, ns, string_mark, row, col)
    wait()
    eq(1, rv)
    local pos = buffer('mark_index', buf, ns, string_mark)
    neq(pos, nil)
    eq(row, pos[1])
    eq(col, pos[2])
    -- Test adding a second mark works
    rv = buffer('mark_set', buf, ns, 'abc', 0, 3)
    wait()
    eq(1, rv)

    -- Test an update
    rv = buffer('mark_set', buf, ns, string_mark, row, col)
    wait()
    eq(2, rv)
    row = 0
    col = col + 1
    rv = buffer('mark_set', buf, ns, string_mark, row, col)
    wait()
    eq(2, rv)
    pos = buffer('mark_index', buf, ns, string_mark)
    eq(row, pos[1])
    eq(col, pos[2])

    -- remove the test marks
    rv = buffer('mark_unset', buf, ns, string_mark)
    eq(1, rv)
    rv = buffer('mark_unset', buf, ns, string_mark)
    -- TODO catch this error..
    eq(0, rv)
    rv = buffer('mark_unset', buf, ns, 'abc')
    eq(1, rv)

    -- add some more marks
    marks = {"a", "b", "c"}
    positions = {{0, 1,}, {0, 3}, {0, 4}}
    for i, m in ipairs(marks) do
        rv = buffer('mark_set', buf, ns, m, positions[i][1], positions[i][2])
        eq(1, rv)
    end
    local names = buffer('mark_names', buf, ns)
    for i, k in ipairs(marks) do
        eq(names[i], k)
    end

    -- Using next
    rv = buffer('mark_next', buf, ns, marks[1])
    eq(marks[2], rv[1])
    eq(positions[2], rv[2])
    rv = buffer('mark_next', buf, ns, marks[2])
    eq(marks[3], rv[1])
    eq(positions[3], rv[2])
    rv = buffer('mark_next', buf, ns, marks[3])
    -- eq('', rv[1]) TODO
    -- eq({-1,-1}, rv[2]) TODO

    -- Using prev
    -- rv = buffer('mark_prev', buf, ns, marks[3])
    -- eq(marks[2], rv[1])
    -- eq(positions[2], rv[2])
    -- rv = buffer('mark_prev', buf, ns, marks[2])
    -- eq(marks[1], rv[1])
    -- eq(positions[1], rv[2])
    -- rv = buffer('mark_next', buf, ns, marks[1])
    -- eq('', rv[1])
    -- eq({-1,-1}, rv[2])

    -- marks move with text inserts?
    -- added_text = "999"
    -- feed('0') -- move cursor to start of line
    -- insert(added_text)
    -- wait()
    -- screen:expect([[
      -- 99^9123              |
      -- ~                   |
      -- ~                   |
                          -- |
    -- ]])
    -- pos = buffer('mark_index', buf, ns, string_mark)
    -- neq(pos, nil)
    -- eq(row, pos[1])
    -- eq(col + string.len(added_text), pos[2])

  end)
end)

