
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

  it('tims', function()
    local buf = helpers.nvim('get_current_buffer')
    local mark_name = 0
    local ns_string = "my-fancy-plugin"
    local ns_string2 = "my-fancy-plugin2"
    local row = 0
    local col = 2

    local init_text = "12345"
    insert(init_text)

    -- setup the namespace
    local rv = buffer('mark_ns_init', ns_string)
    eq(1, rv)
    -- rv = buffer('mark_ns_init', ns_string)
    -- eq(0, rv)
    rv = buffer('mark_ns_init', ns_string2)
    eq(2, rv)

    rv = buffer('mark_ns_ids')
    eq({ns_string, 1}, rv[1])
    eq({ns_string2, 2}, rv[2])


    local ns = 1
    -- mark the "2"
    rv = buffer('mark_set', buf, ns, mark_name, row, col)
    wait()
    eq(1, rv)
    rv = buffer('mark_index', buf, ns, mark_name)
    eq({mark_name, row, col}, rv)
    -- Test adding a second mark works
    local mark_name2 = 1
    rv = buffer('mark_set', buf, ns, mark_name2, 0, 3)
    wait()
    eq(1, rv)

    -- Test an update
    rv = buffer('mark_set', buf, ns, mark_name, row, col)
    wait()
    eq(2, rv)
    row = 0
    col = col + 1
    rv = buffer('mark_set', buf, ns, mark_name, row, col)
    wait()
    eq(2, rv)
    rv = buffer('mark_index', buf, ns, mark_name)
    eq({mark_name, row, col}, rv)

    -- remove the test marks
    rv = buffer('mark_unset', buf, ns, mark_name)
    eq(1, rv)
    rv = buffer('mark_unset', buf, ns, mark_name2)
    eq(1, rv)

    -- add some more marks
    marks = {2, 3, 4}
    positions = {{0, 1,}, {0, 3}, {0, 4}}
    for i, m in ipairs(marks) do
        rv = buffer('mark_set', buf, ns, m, positions[i][1], positions[i][2])
        eq(1, rv)
    end
    rv = buffer('mark_ids', buf, ns)
    for i, m in ipairs(marks) do
        eq({m, positions[i][1], positions[i][2]}, rv[i])
    end

    -- Using next
    rv = buffer('mark_next', buf, ns, marks[1])
    eq({marks[2], positions[2][1], positions[2][2]}, rv)
    rv = buffer('mark_next', buf, ns, marks[2])
    eq({marks[3], positions[3][1], positions[3][2]}, rv)
    rv = buffer('mark_next', buf, ns, marks[3])
    eq({-1, -1, -1}, rv)
    -- next range
    rv = buffer('mark_nextrange', buf, ns, marks[1], marks[3])
    eq({marks[1], positions[1][1], positions[1][2]}, rv[1])
    eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    eq({marks[3], positions[3][1], positions[3][2]}, rv[3])

    -- Using prev
    -- rv = buffer('mark_prev', buf, ns, marks[3])
    -- eq({marks[2], positions[2][1], positions[2][2]}, rv)
    -- rv = buffer('mark_prev', buf, ns, marks[2])
    -- eq({marks[1], positions[1][1], positions[1][2]}, rv)
    -- eq(positions[1], rv[2])
    -- rv = buffer('mark_prev', buf, ns, marks[1])
    -- eq({-1, -1, -1}, rv)
    -- prev range
    -- rv = buffer('mark_prevrange', buf, ns, marks[1], marks[3])
    -- eq({marks[3], positions[3][1], positions[3][2]}, rv[1])
    -- eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    -- eq({marks[1], positions[1][1], positions[1][2]}, rv[3])

    -- index, next, prev, nextrange, prevrange, work with positional indexes
    rv = buffer('mark_index', buf, ns, positions[1])
    eq({marks[1], positions[1][1], positions[1][2]}, rv)

    -- TODO decide on behaviour
    -- rv = buffer('mark_next', buf, ns, positions[1])
    -- eq({marks[1], positions[1][1], positions[1][2]}, rv)

    rv = buffer('mark_nextrange', buf, ns, positions[1], positions[3])
    eq({marks[1], positions[1][1], positions[1][2]}, rv[1])
    eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    eq({marks[3], positions[3][1], positions[3][2]}, rv[3])

    -- TODO decide on behaviour
    -- rv = buffer('mark_prev', buf, ns, positions[3])
    -- eq({marks[1], positions[1][1], positions[1][2]}, rv)

    -- rv = buffer('mark_prevrange', buf, ns, positions[1], positions[3])
    -- eq({marks[3], positions[3][1], positions[3][2]}, rv[1])
    -- eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    -- eq({marks[1], positions[1][1], positions[1][2]}, rv[3])

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
    -- pos = buffer('mark_index', buf, ns, mark_name)
    -- neq(pos, nil)
    -- eq(row, pos[1])
    -- eq(col + string.len(added_text), pos[2])

  end)
end)

