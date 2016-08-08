
local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')

local request = helpers.request
local eq = helpers.eq
local neq = helpers.neq
local buffer = helpers.buffer
local insert = helpers.insert
local feed = helpers.feed
local wait = helpers.wait

describe('Extmarks buffer api', function()
  local screen
  local curbuf
  local marks, positions, ns_string2, ns_string, init_text, row, col
  local ns

  before_each(function()
  marks = {2, 3, 4}
  positions = {{0, 1,}, {0, 3}, {0, 4}}

  ns_string = "my-fancy-plugin"
  ns_string2 = "my-fancy-plugin2"
  init_text = "12345"
  row = 0
  col = 2

  ns = 0

    helpers.clear()
    helpers.nvim('set_option', 'shell', helpers.nvim_dir .. '/shell-test')
    helpers.nvim('set_option', 'shellcmdflag', 'EXE')
    screen = Screen.new(20, 4)
    screen:attach(false)

    insert(init_text)
  buf = request('vim_get_current_buffer')
  end)


  -- after_each(function()
    -- screen:detach()
  -- end)

  it('add and query namespaces', function()
    rv = buffer('mark_ns_init', ns_string)
    eq(1, rv)
    -- rv = buffer('mark_ns_init', ns_string)
    -- eq(0, rv)
    rv = buffer('mark_ns_init', ns_string2)
    eq(2, rv)

    rv = buffer('mark_ns_ids')
    eq({ns_string, 1}, rv[1])
    eq({ns_string2, 2}, rv[2])
  -- end)

  -- it('adds, updates  and deletes marks', function()
    -- ns = buffer('mark_ns_init', ns_string)
    rv = buffer('mark_set', buf, ns, marks[1], positions[1][1], positions[1][2])
    eq(1, rv)
    rv = buffer('mark_index', buf, ns, marks[1])
    eq({marks[1], positions[1][1], positions[1][2]}, rv)
    -- Test adding a second mark works
    rv = buffer('mark_set', buf, ns, marks[2], positions[2][1], positions[2][2])
    wait()
    eq(1, rv)
    -- Test an update, (same pos)
    rv = buffer('mark_set', buf, ns, marks[1], positions[1][1], positions[1][2])
    eq(2, rv)
    rv = buffer('mark_index', buf, ns, marks[2])
    eq({marks[2], positions[2][1], positions[2][2]}, rv)
    -- Test an update, (new pos)
    row = positions[1][1]
    col = positions[1][2] + 1
    rv = buffer('mark_set', buf, ns, marks[1], row, col)
    eq(2, rv)
    rv = buffer('mark_index', buf, ns, marks[1])
    eq({marks[1], row, col}, rv)

    -- remove the test marks
    rv = buffer('mark_unset', buf, ns, marks[1])
    eq(1, rv)
    rv = buffer('mark_unset', buf, ns, marks[2])
    eq(1, rv)
  -- end)

  -- it('gets information on current marks ', function()
    -- add some more marks
    for i, m in ipairs(marks) do
        rv = buffer('mark_set', buf, ns, m, positions[i][1], positions[i][2])
        eq(1, rv)
    end
    rv = buffer('mark_ids', buf, ns)
    for i, m in ipairs(marks) do
        eq({m, positions[i][1], positions[i][2]}, rv[i])
    end
  -- end)

  -- it('returns ranges correctly', function()
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
    -- end)

  -- it('returns ranges correctly when using position indexes', function()
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
  end)

  pending('marks move with line insertations #test', function()
    rv = buffer('mark_set', buf, ns, marks[1], 1, 1)
    eq(1, rv)
    screen:snapshot_util()
    feed("yyP")
    screen:snapshot_util()
    added_text = "999"
    --feed("i<cr><esc>")
    screen:snapshot_util()
    rv = buffer('mark_index', buf, ns, marks[1])
    eq({2, 2, 1}, rv)
  end)

  pending('marks move with line join #test', function()
    feed("i<cr>abc<esc>")
    screen:snapshot_util()
    rv = buffer('mark_set', buf, ns, marks[1], 2, 1)
    added_text = "999"
    feed('ggJ') -- move cursor to start of line
    --feed("i<cr><esc>")
    screen:snapshot_util()
    rv = buffer('mark_index', buf, ns, marks[1])
    print(require('inspect')(rv))
    eq(row, pos[1])
    eq(col + string.len(added_text), pos[2])
  end)

  pending('marks move with line splits #test', function()
    screen:snapshot_util()
    rv = buffer('mark_set', buf, ns, marks[1], 1, 2)
    added_text = "999"
    feed('0li<cr><esc>') -- move cursor to start of line
    --feed("i<cr><esc>")
    screen:snapshot_util()
    rv = buffer('mark_index', buf, ns, marks[1])
    print(require('inspect')(rv))
    eq(row, pos[1])
    eq(col + string.len(added_text), pos[2])
  end)

  pending('marks move with text inserts', function()
    rv = buffer('mark_set', buf, ns, marks[1], 0, 1)
    added_text = "999"
    feed('0') -- move cursor to start of line
    insert(added_text)
    screen:expect([[
      99^912345            |
      ~                   |
      ~                   |
                          |
    ]])
    rv = buffer('mark_index', buf, ns, marks[1])
    print(require('inspect')(pos))
    eq(row, pos[1])
    eq(col + string.len(added_text), pos[2])
  end)

end)

