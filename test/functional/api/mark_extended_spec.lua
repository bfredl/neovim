-- TODO(timeyyy): go through todo's lol
-- change representation of stored marks to have location start at 0
-- check with memsan, asan etc

local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')

local request = helpers.request
local eq = helpers.eq
local curbufmeths = helpers.curbufmeths
local insert = helpers.insert
local feed = helpers.feed
local clear = helpers.clear

local TO_START = {-1, -1}
local TO_END = {-1, -1}
local ALL = -1

local rv = nil

local function check_undo_redo(ns, mark, sr, sc, er, ec) --s = start, e = end
  rv = curbufmeths.get_extmark(ns, mark)
  eq({er, ec}, rv)
  feed("u")
  rv = curbufmeths.get_extmark(ns, mark)
  eq({sr, sc}, rv)
  feed("<c-r>")
  rv = curbufmeths.get_extmark(ns, mark)
  eq({er, ec}, rv)
end

describe('Extmarks buffer api', function()
  local screen
  local marks, positions, ns_string2, ns_string, init_text, row, col
  local ns, ns2

  before_each(function()
    -- Initialize some namespaces and insert 12345 into a buffer
    marks = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}
    positions = {{1, 1,}, {1, 3}, {1, 4}}

    ns_string = "my-fancy-plugin"
    ns_string2 = "my-fancy-plugin2"
    init_text = "12345"
    row = 0
    col = 2

    clear()
    screen = Screen.new(15, 10)
    screen:attach()

    insert(init_text)
    ns = request('nvim_create_namespace', ns_string)
    ns2 = request('nvim_create_namespace', ns_string2)
  end)

  it('adds, updates  and deletes marks #extmarks', function()
    rv = curbufmeths.set_extmark(ns, marks[1], positions[1][1], positions[1][2])
    eq(marks[1], rv)
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({positions[1][1], positions[1][2]}, rv)
    -- Test adding a second mark on same row works
    rv = curbufmeths.set_extmark(ns, marks[2], positions[2][1], positions[2][2])
    eq(marks[2], rv)
    -- Test a second mark on a different line
    rv = curbufmeths.set_extmark(ns, marks[3], positions[1][1] + 1, positions[1][2])
    eq(marks[3], rv)
    -- Test an update, (same pos)
    rv = curbufmeths.set_extmark(ns, marks[1], positions[1][1], positions[1][2])
    eq(0, rv)
    rv = curbufmeths.get_extmark(ns, marks[2])
    eq({positions[2][1], positions[2][2]}, rv)
    -- Test an update, (new pos)
    row = positions[1][1]
    col = positions[1][2] + 1
    rv = curbufmeths.set_extmark(ns, marks[1], row, col)
    eq(0, rv)
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({row, col}, rv)

    -- remove the test marks
    eq(true, curbufmeths.del_extmark(ns, marks[1]))
    eq(false, curbufmeths.del_extmark(ns, marks[1]))
    eq(true, curbufmeths.del_extmark(ns, marks[2]))
    eq(true, curbufmeths.del_extmark(ns, marks[3]))
    eq(false, curbufmeths.del_extmark(ns, 1000))
  end)

  it('querying for information and ranges #extmarks', function()
    -- add some more marks
    for i, m in ipairs(marks) do
      if positions[i] ~= nil then
        rv = curbufmeths.set_extmark(ns, m, positions[i][1], positions[i][2])
        eq(m, rv)
      end
    end

    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)
    for i, m in ipairs(marks) do
      if positions[i] ~= nil then
        eq({m, positions[i][1], positions[i][2]}, rv[i])
      end
    end

    -- next with mark id
    rv = curbufmeths.list_extmarks(ns, marks[1], TO_END, 1, 0)
    eq({{marks[1], positions[1][1], positions[1][2]}}, rv)
    rv = curbufmeths.list_extmarks(ns, marks[2], TO_END, 1, 0)
    eq({{marks[2], positions[2][1], positions[2][2]}}, rv)
    -- next with positional when mark exists at position
    rv = curbufmeths.list_extmarks(ns, positions[1], TO_END, 1, 0)
    eq({{marks[1], positions[1][1], positions[1][2]}}, rv)
    -- next with positional index (no mark at position)
    rv = curbufmeths.list_extmarks(ns, {positions[1][1], positions[1][2] +1}, TO_END, 1, 0)
    eq({{marks[2], positions[2][1], positions[2][2]}}, rv)
    -- next with Extremity index
    rv = curbufmeths.list_extmarks(ns, -1, -1, 1, 0)
    eq({{marks[1], positions[1][1], positions[1][2]}}, rv)

    -- nextrange with mark id
    rv = curbufmeths.list_extmarks(ns, marks[1], marks[3], ALL, 0)
    eq({marks[1], positions[1][1], positions[1][2]}, rv[1])
    eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    eq({marks[3], positions[3][1], positions[3][2]}, rv[3])
    -- nextrange with amount
    rv = curbufmeths.list_extmarks(ns, marks[1], marks[3], 2, 0)
    eq(2, table.getn(rv))
    -- nextrange with positional when mark exists at position
    rv = curbufmeths.list_extmarks(ns, positions[1], positions[3], ALL, 0)
    eq({marks[1], positions[1][1], positions[1][2]}, rv[1])
    eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    eq({marks[3], positions[3][1], positions[3][2]}, rv[3])
    rv = curbufmeths.list_extmarks(ns, positions[2], positions[3], ALL, 0)
    eq(2, table.getn(rv))
    -- nextrange with positional index (no mark at position)
    local lower = {positions[1][1], positions[2][2] -1}
    local upper = {positions[2][1], positions[3][2] - 1}
    rv = curbufmeths.list_extmarks(ns, lower, upper, ALL, 0)
    eq({{marks[2], positions[2][1], positions[2][2]}}, rv)
    lower = {positions[3][1], positions[3][2] + 1}
    upper = {positions[3][1], positions[3][2] + 2}
    rv = curbufmeths.list_extmarks(ns, lower, upper, ALL, 0)
    eq({}, rv)
    -- nextrange with extremity index
    lower = {positions[2][1], positions[2][2]+1}
    upper = -1
    rv = curbufmeths.list_extmarks(ns, lower, upper, ALL, 0)
    eq({{marks[3], positions[3][1], positions[3][2]}}, rv)

    -- prev with mark id
    rv = curbufmeths.list_extmarks(ns, TO_START, marks[3], 1, 1)
    eq({{marks[3], positions[3][1], positions[3][2]}}, rv)
    rv = curbufmeths.list_extmarks(ns, TO_START, marks[2], 1, 1)
    eq({{marks[2], positions[2][1], positions[2][2]}}, rv)
    -- prev with positional when mark exists at position
    rv = curbufmeths.list_extmarks(ns, TO_START, positions[3], 1, 1)
    eq({{marks[3], positions[3][1], positions[3][2]}}, rv)
    -- prev with positional index (no mark at position)
    rv = curbufmeths.list_extmarks(ns, TO_START, {positions[1][1], positions[1][2] +1}, 1, 1)
    eq({{marks[1], positions[1][1], positions[1][2]}}, rv)
    -- prev with Extremity index
    rv = curbufmeths.list_extmarks(ns, -1, -1, 1, 1)
    eq({{marks[3], positions[3][1], positions[3][2]}}, rv)

    -- prevrange with mark id
    rv = curbufmeths.list_extmarks(ns, marks[1], marks[3], ALL, 1)
    eq({marks[3], positions[3][1], positions[3][2]}, rv[1])
    eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    eq({marks[1], positions[1][1], positions[1][2]}, rv[3])
    -- prevrange with amount
    rv = curbufmeths.list_extmarks(ns, marks[1], marks[3], 2, 1)
    eq(2, table.getn(rv))
    -- prevrange with positional when mark exists at position
    rv = curbufmeths.list_extmarks(ns, positions[1], positions[3], ALL, 1)
    eq({marks[3], positions[3][1], positions[3][2]}, rv[1])
    eq({marks[2], positions[2][1], positions[2][2]}, rv[2])
    eq({marks[1], positions[1][1], positions[1][2]}, rv[3])
    rv = curbufmeths.list_extmarks(ns, positions[1], positions[2], ALL, 1)
    eq(2, table.getn(rv))
    -- prevrange with positional index (no mark at position)
    lower = {positions[2][1], positions[2][2] + 1}
    upper = {positions[3][1], positions[3][2] + 1}
    rv = curbufmeths.list_extmarks(ns, lower, upper, ALL, 1)
    eq({{marks[3], positions[3][1], positions[3][2]}}, rv)
    lower = {positions[3][1], positions[3][2] + 1}
    upper = {positions[3][1], positions[3][2] + 2}
    rv = curbufmeths.list_extmarks(ns, lower, upper, ALL, 1)
    eq({}, rv)
    -- prevrange with extremity index
    lower = -1
    upper = {positions[2][1], positions[2][2] - 1}
    rv = curbufmeths.list_extmarks(ns, lower, upper, ALL, 1)
    eq({{marks[1], positions[1][1], positions[1][2]}}, rv)
  end)

  it('querying for information with amount #extmarks', function()
    -- add some more marks
    for i, m in ipairs(marks) do
      if positions[i] ~= nil then
        rv = curbufmeths.set_extmark(ns, m, positions[i][1], positions[i][2])
        eq(m, rv)
      end
    end

    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, 1, 0)
    eq(1, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, 2, 0)
    eq(2, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, 3, 0)
    eq(3, table.getn(rv))

    -- now in reverse
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, 1, 0)
    eq(1, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, 2, 0)
    eq(2, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, 3, 0)
    eq(3, table.getn(rv))
  end)

  it('get_marks works when mark col > upper col #extmarks', function()
    feed('A<cr>12345<esc>')
    feed('A<cr>12345<esc>')
    curbufmeths.set_extmark(ns, 10, 1, 3)       -- this shouldn't be found
    curbufmeths.set_extmark(ns, 11, 3, 2)       -- this shouldn't be found
    curbufmeths.set_extmark(ns, marks[1], 1, 5) -- check col > our upper bound
    curbufmeths.set_extmark(ns, marks[2], 2, 2) -- check col < lower bound
    curbufmeths.set_extmark(ns, marks[3], 3, 1) -- check is inclusive
    rv = curbufmeths.list_extmarks(ns, {1, 4}, {3, 1}, -1, 0)
    eq({{marks[1], 1, 5},
        {marks[2], 2, 2},
        {marks[3], 3, 1}},
       rv)
  end)

  it('get_marks works in reverse when mark col < lower col #extmarks', function()
    feed('A<cr>12345<esc>')
    feed('A<cr>12345<esc>')
    curbufmeths.set_extmark(ns, 10, 1, 2) -- this shouldn't be found
    curbufmeths.set_extmark(ns, 11, 3, 5) -- this shouldn't be found
    curbufmeths.set_extmark(ns, marks[1], 3, 2) -- check col < our lower bound
    curbufmeths.set_extmark(ns, marks[2], 2, 5) -- check col > upper bound
    curbufmeths.set_extmark(ns, marks[3], 1, 3) -- check is inclusive
    rv = curbufmeths.list_extmarks(ns, {1, 3}, {3, 4}, -1, 1)
    eq({{marks[1], 3, 2},
        {marks[2], 2, 5},
        {marks[3], 1, 3}},
       rv)
  end)

  it('get_marks amount 0 returns nothing #extmarks', function()
    curbufmeths.set_extmark(ns, marks[1], positions[1][1], positions[1][2])
    rv = curbufmeths.list_extmarks(ns, {-1, -1}, {-1, -1}, 0, 0)
    eq({}, rv)
  end)


  it('marks move with line insertations #extmarks', function()
    curbufmeths.set_extmark(ns, marks[1], 1, 1)
    feed("yyP")
    check_undo_redo(ns, marks[1], 1, 1, 2, 1)
  end)

  it('marks move with multiline insertations #extmarks', function()
    feed("a<cr>22<cr>33<esc>")
    curbufmeths.set_extmark(ns, marks[1], 2, 2)
    feed('ggVGyP')
    check_undo_redo(ns, marks[1], 2, 2, 5, 2)
  end)

  it('marks move with line join #extmarks', function()
    -- do_join in ops.c
    feed("a<cr>222<esc>")
    curbufmeths.set_extmark(ns, marks[1], 2, 1)
    feed('ggJ')
    check_undo_redo(ns, marks[1], 2, 1, 1, 7)
  end)

  it('join works when no marks are present #extmarks', function()
    feed("a<cr>1<esc>")
    feed('kJ')
    -- This shouldn't seg fault
    screen:expect([[
      12345^ 1        |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
                     |
    ]])
  end)

  it('marks move with multiline join #extmarks', function()
    -- do_join in ops.c
    feed("a<cr>222<cr>333<cr>444<esc>")
    curbufmeths.set_extmark(ns, marks[1], 4, 1)
    feed('2GVGJ')
    check_undo_redo(ns, marks[1], 4, 1, 2, 9)
  end)

  it('marks move with line deletes #extmarks', function()
    feed("a<cr>222<cr>333<cr>444<esc>")
    curbufmeths.set_extmark(ns, marks[1], 3, 2)
    feed('ggjdd')
    check_undo_redo(ns, marks[1], 3, 2, 2, 2)
  end)

  it('marks move with multiline deletes #extmarks', function()
    feed("a<cr>222<cr>333<cr>444<esc>")
    curbufmeths.set_extmark(ns, marks[1], 4, 1)
    feed('gg2dd')
    check_undo_redo(ns, marks[1], 4, 1, 2, 1)
    -- regression test, undoing multiline delete when mark is on row 1
    feed('ugg3dd')
    check_undo_redo(ns, marks[1], 4, 1, 1, 1)
  end)

  it('marks move with open line #extmarks', function()
    -- open_line in misc1.c
    -- testing marks below are also moved
    feed("yyP")
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    curbufmeths.set_extmark(ns, marks[2], 2, 5)
    feed('1G<s-o><esc>')
    check_undo_redo(ns, marks[1], 1, 5, 2, 5)
    check_undo_redo(ns, marks[2], 2, 5, 3, 5)
    feed('2Go<esc>')
    check_undo_redo(ns, marks[1], 2, 5, 2, 5)
    check_undo_redo(ns, marks[2], 3, 5, 4, 5)
  end)

  -- NO idea why this doesn't work... works in program
  pending('marks move with char inserts #extmarks', function()
    -- insertchar in edit.c (the ins_str branch)
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('0')
    insert('abc')
    screen:expect([[
      ab^c12345       |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
      ~              |
                     |
    ]])
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq(1, rv[2])
    eq(6, rv[3])
    -- check_undo_redo(ns, marks[1], 1, 3, 1, 6)
  end)

  -- gravity right as definted in tk library
  it('marks have gravity right #extmarks', function()
    -- insertchar in edit.c (the ins_str branch)
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('03l')
    insert("X")
    check_undo_redo(ns, marks[1], 1, 3, 1, 3)

    -- check multibyte chars
    feed('03l<esc>')
    insert("～～")
    check_undo_redo(ns, marks[1], 1, 3, 1, 3)
  end)

  it('we can insert multibyte chars #extmarks', function()
    -- insertchar in edit.c
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 3)
    -- Insert a fullwidth (two col) tilde, NICE
    feed('0i～<esc>')
    check_undo_redo(ns, marks[1], 2, 3, 2, 4)
  end)

  it('marks move with blockwise inserts #extmarks', function()
    -- op_insert in ops.c
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 3)
    feed('0<c-v>lkI9<esc>')
    check_undo_redo(ns, marks[1], 2, 3, 2, 4)
  end)

  it('marks move with line splits (using enter) #extmarks', function()
    -- open_line in misc1.c
    -- testing marks below are also moved
    feed("yyP")
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    curbufmeths.set_extmark(ns, marks[2], 2, 5)
    feed('1Gla<cr><esc>')
    check_undo_redo(ns, marks[1], 1, 5, 2, 3)
    check_undo_redo(ns, marks[2], 2, 5, 3, 5)
  end)

  -- TODO mark_col_adjust for normal marks fails in vim/neovim
  -- because flags is 9 in: if (flags & OPENLINE_MARKFIX) {
  it('marks at last line move on insert new line #extmarks', function()
    -- open_line in misc1.c
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    feed('0i<cr><esc>')
    check_undo_redo(ns, marks[1], 1, 5, 2, 5)
  end)

  it('yet again marks move with line splits #extmarks', function()
    -- the first test above wasn't catching all errors..
    feed("A67890<esc>")
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    feed("04li<cr><esc>")
    check_undo_redo(ns, marks[1], 1, 5, 2, 1)
  end)

  it('and one last time line splits... #extmarks', function()
    curbufmeths.set_extmark(ns, marks[1], 1, 2)
    curbufmeths.set_extmark(ns, marks[2], 1, 3)
    feed("02li<cr><esc>")
    check_undo_redo(ns, marks[1], 1, 2, 1, 2)
    check_undo_redo(ns, marks[2], 1, 3, 2, 1)
  end)

  it('multiple marks move with mark splits #extmarks', function()
    curbufmeths.set_extmark(ns, marks[1], 1, 2)
    curbufmeths.set_extmark(ns, marks[2], 1, 4)
    feed("0li<cr><esc>")
    check_undo_redo(ns, marks[1], 1, 2, 2, 1)
    check_undo_redo(ns, marks[2], 1, 4, 2, 3)
  end)

  it('deleting on a mark works #extmarks', function()
    -- op_delete in ops.c
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('02lx')
    check_undo_redo(ns, marks[1], 1, 3, 1, 3)
  end)

  it('marks move with char deletes #extmarks', function()
    -- op_delete in ops.c
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('02dl')
    check_undo_redo(ns, marks[1], 1, 3, 1, 1)
    -- from the other side (nothing should happen)
    feed('$x')
    check_undo_redo(ns, marks[1], 1, 1, 1, 1)
  end)

  it('marks move with char deletes over a range #extmarks', function()
    -- op_delete in ops.c
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    curbufmeths.set_extmark(ns, marks[2], 1, 4)
    feed('0l3dl<esc>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 2)
    check_undo_redo(ns, marks[2], 1, 4, 1, 2)
    -- delete 1, nothing should happend to our marks
    feed('u')
    feed('$x')
    -- TODO do we need to test marks[1] ???
    check_undo_redo(ns, marks[2], 1, 4, 1, 4)
  end)

  it('deleting marks at end of line works #extmarks', function()
    -- mark_extended.c/extmark_col_adjust_delete
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    feed('$x')
    check_undo_redo(ns, marks[1], 1, 5, 1, 5)
    -- check the copy happened correctly on delete at eol
    feed('$x')
    check_undo_redo(ns, marks[1], 1, 5, 1, 4)
    feed('u')
    check_undo_redo(ns, marks[1], 1, 5, 1, 5)
  end)

  it('marks move with blockwise deletes #extmarks', function()
    -- op_delete in ops.c
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 5)
    feed('h<c-v>hhkd')
    check_undo_redo(ns, marks[1], 2, 5, 2, 2)
  end)

  it('marks move with blockwise deletes over a range #extmarks', function()
    -- op_delete in ops.c
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 2)
    curbufmeths.set_extmark(ns, marks[2], 1, 4)
    curbufmeths.set_extmark(ns, marks[3], 2, 3)
    feed('0<c-v>k3lx')
    check_undo_redo(ns, marks[1], 1, 2, 1, 1)
    check_undo_redo(ns, marks[2], 1, 4, 1, 1)
    check_undo_redo(ns, marks[3], 2, 3, 2, 1)
    -- delete 1, nothing should happend to our marks
    feed('u')
    feed('$<c-v>jx')
    -- TODO do we need to test marks[1] ???
    check_undo_redo(ns, marks[2], 1, 4, 1, 4)
    check_undo_redo(ns, marks[3], 2, 3, 2, 3)
  end)

  it('works with char deletes over multilines #extmarks', function()
    feed('a<cr>12345<cr>test-me<esc>')
    curbufmeths.set_extmark(ns, marks[1], 3, 6)
    feed('gg')
    feed('dv?-m?<cr>')
    check_undo_redo(ns, marks[1], 3, 6, 1, 1)
  end)

  it('marks outside of deleted range move with visual char deletes #extmarks', function()
    -- op_delete in ops.c
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    feed('0vx<esc>')
    check_undo_redo(ns, marks[1], 1, 4, 1, 3)

    feed("u")
    feed('0vlx<esc>')
    check_undo_redo(ns, marks[1], 1, 4, 1, 2)

    feed("u")
    feed('0v2lx<esc>')
    check_undo_redo(ns, marks[1], 1, 4, 1, 1)

    -- from the other side (nothing should happen)
    feed('$vx')
    check_undo_redo(ns, marks[1], 1, 1, 1, 1)
  end)

  it('marks outside of deleted range move with char deletes #extmarks', function()
    -- op_delete in ops.c
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    feed('0x<esc>')
    check_undo_redo(ns, marks[1], 1, 4, 1, 3)

    feed("u")
    feed('02x<esc>')
    check_undo_redo(ns, marks[1], 1, 4, 1, 2)

    feed("u")
    feed('0v3lx<esc>')
    check_undo_redo(ns, marks[1], 1, 4, 1, 1)

    -- from the other side (nothing should happen)
    feed("u")
    feed('$vx')
    check_undo_redo(ns, marks[1], 1, 4, 1, 4)
  end)

  it('marks move with P(backward) paste #extmarks', function()
    -- do_put in ops.c
    feed('0iabc<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 8)
    feed('0veyP')
    check_undo_redo(ns, marks[1], 1, 8, 1, 16)
  end)

  it('marks move with p(forward) paste #extmarks', function()
    -- do_put in ops.c
    feed('0iabc<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 8)
    feed('0veyp')
    check_undo_redo(ns, marks[1], 1, 8, 1, 15)
  end)

  it('marks move with blockwise P(backward) paste #extmarks', function()
    -- do_put in ops.c
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 5)
    feed('<c-v>hhkyP<esc>')
    check_undo_redo(ns, marks[1], 2, 5, 2, 8)
  end)

  it('marks move with blockwise p(forward) paste #extmarks', function()
    -- do_put in ops.c
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 5)
    feed('<c-v>hhkyp<esc>')
    check_undo_redo(ns, marks[1], 2, 5, 2, 7)
  end)

  it('replace works #extmarks', function()
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('0r2')
    check_undo_redo(ns, marks[1], 1, 3, 1, 3)
  end)

  it('blockwise replace works #extmarks', function()
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('0<c-v>llkr1<esc>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 3)
  end)

  it('shift line #extmarks', function()
    -- shift_line in ops.c
    feed(':set shiftwidth=4<cr><esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('0>>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 7)

    feed('>>')
    check_undo_redo(ns, marks[1], 1, 7, 1, 11)

    feed('<LT><LT>') -- have to escape, same as <<
    check_undo_redo(ns, marks[1], 1, 11, 1, 7)
  end)

  it('blockwise shift #extmarks', function()
    -- shift_block in ops.c
    feed(':set shiftwidth=4<cr><esc>')
    feed('a<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 3)
    feed('0<c-v>k>')
    check_undo_redo(ns, marks[1], 2, 3, 2, 7)
    feed('<c-v>j>')
    check_undo_redo(ns, marks[1], 2, 7, 2, 11)

    feed('<c-v>j<LT>')
    check_undo_redo(ns, marks[1], 2, 11, 2, 7)
  end)

  it('tab works with expandtab #extmarks', function()
    -- ins_tab in edit.c
    feed(':set expandtab<cr><esc>')
    feed(':set shiftwidth=2<cr><esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('0i<tab><tab><esc>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 7)
  end)

  it('tabs work #extmarks', function()
    -- ins_tab in edit.c
    feed(':set noexpandtab<cr><esc>')
    feed(':set shiftwidth=2<cr><esc>')
    feed(':set softtabstop=2<cr><esc>')
    feed(':set tabstop=8<cr><esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    feed('0i<tab><esc>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 5)
    feed('0iX<tab><esc>')
    check_undo_redo(ns, marks[1], 1, 5, 1, 7)
  end)

  it('marks move when using :move #extmarks', function()
    curbufmeths.set_extmark(ns, marks[1], 1, 1)
    feed('A<cr>2<esc>:1move 2<cr><esc>')
    check_undo_redo(ns, marks[1], 1, 1, 2, 1)
    -- test codepath when moving lines up
    feed(':2move 0<cr><esc>')
    check_undo_redo(ns, marks[1], 2, 1, 1, 1)
  end)

  it('marks move when using :move part 2 #extmarks', function()
    -- make sure we didn't get lucky with the math...
    feed('A<cr>2<cr>3<cr>4<cr>5<cr>6<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 1)
    feed(':2,3move 5<cr><esc>')
    check_undo_redo(ns, marks[1], 2, 1, 4, 1)
    -- test codepath when moving lines up
    feed(':4,5move 1<cr><esc>')
    check_undo_redo(ns, marks[1], 4, 1, 2, 1)
  end)

  it('undo and redo of set and unset marks #extmarks', function()
    -- Force a new undo head
    feed('o<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 2)
    feed('o<esc>')
    curbufmeths.set_extmark(ns, marks[2], 1, 8)
    curbufmeths.set_extmark(ns, marks[3], 1, 9)
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)

    feed("u")
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)
    eq(1, table.getn(rv))

    feed("<c-r>")
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)
    eq(3, table.getn(rv))

    -- Test updates
    feed('o<esc>')
    curbufmeths.set_extmark(ns, marks[1], positions[1][1], positions[1][2])
    rv = curbufmeths.list_extmarks(ns, marks[1], marks[1], 1, 0)
    feed("u")
    feed("<c-r>")
    check_undo_redo(ns, marks[1], 1, 2, positions[1][1], positions[1][2])

    -- Test unset
    feed('o<esc>')
    curbufmeths.del_extmark(ns, marks[3])
    feed("u")
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)
    eq(3, table.getn(rv))
    feed("<c-r>")
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)
    eq(2, table.getn(rv))
  end)

  it('undo and redo of marks deleted during edits #extmarks', function()
    -- test extmark_adjust
    feed('A<cr>12345<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 3)
    feed('dd')
    rv = curbufmeths.get_extmark(ns, marks[1])
    check_undo_redo(ns, marks[1], 2, 3, 1, 6)
  end)

  it('namespaces work properly #extmarks', function()
    rv = curbufmeths.set_extmark(ns, marks[1], positions[1][1], positions[1][2])
    eq(1, rv)
    rv = curbufmeths.set_extmark(ns2, marks[1], positions[1][1], positions[1][2])
    eq(1, rv)
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)
    eq(1, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns2, TO_START, TO_END, ALL, 0)
    eq(1, table.getn(rv))

    -- Set more marks for testing the ranges
    rv = curbufmeths.set_extmark(ns, marks[2], positions[2][1], positions[2][2])
    rv = curbufmeths.set_extmark(ns, marks[3], positions[3][1], positions[3][2])
    rv = curbufmeths.set_extmark(ns2, marks[2], positions[2][1], positions[2][2])
    rv = curbufmeths.set_extmark(ns2, marks[3], positions[3][1], positions[3][2])

    -- get_next (amount set)
    rv = curbufmeths.list_extmarks(ns, TO_START, positions[2], 1, 0)
    eq(1, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns2, TO_START, positions[2], 1, 0)
    eq(1, table.getn(rv))
    -- get_prev (amount set)
    rv = curbufmeths.list_extmarks(ns, TO_START, positions[1], 1, 1)
    eq(1, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns2, TO_START, positions[1], 1, 1)
    eq(1, table.getn(rv))

    -- get_next (amount not set)
    rv = curbufmeths.list_extmarks(ns, positions[1], positions[2], ALL, 0)
    eq(2, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns2, positions[1], positions[2], ALL, 0)
    eq(2, table.getn(rv))
    -- get_prev (amount not set)
    rv = curbufmeths.list_extmarks(ns, positions[1], positions[2], ALL, 1)
    eq(2, table.getn(rv))
    rv = curbufmeths.list_extmarks(ns2, positions[1], positions[2], ALL, 1)
    eq(2, table.getn(rv))

    curbufmeths.del_extmark(ns, marks[1])
    rv = curbufmeths.list_extmarks(ns, TO_START, TO_END, ALL, 0)
    eq(2, table.getn(rv))
    curbufmeths.del_extmark(ns2, marks[1])
    rv = curbufmeths.list_extmarks(ns2, TO_START, TO_END, ALL, 0)
    eq(2, table.getn(rv))
  end)

  it('mark set can create unique identifiers #extmarks', function()
    -- create mark with id 1
    eq(1, curbufmeths.set_extmark(ns, 1, positions[1][1], positions[1][2]))
    -- ask for unique id, it should be the next one, i e 2
    eq(2, curbufmeths.set_extmark(ns, 0, positions[1][1], positions[1][2]))
    eq(3, curbufmeths.set_extmark(ns, 3, positions[2][1], positions[2][2]))
    eq(4, curbufmeths.set_extmark(ns, 0, positions[1][1], positions[1][2]))

    -- mixing manual and allocated id:s are not recommened, but it should
    -- do something reasonable
    eq(6, curbufmeths.set_extmark(ns, 6, positions[2][1], positions[2][2]))
    eq(7, curbufmeths.set_extmark(ns, 0, positions[1][1], positions[1][2]))
    eq(8, curbufmeths.set_extmark(ns, 0, positions[1][1], positions[1][2]))
  end)

  it('auto indenting with enter works #extmarks', function()
    -- op_reindent in ops.c
    feed(':set cindent<cr><esc>')
    feed(':set autoindent<cr><esc>')
    feed(':set shiftwidth=2<cr><esc>')
    feed("0iint <esc>A {1M1<esc>b<esc>")
    -- Set the mark on the M, should move..
    curbufmeths.set_extmark(ns, marks[1], 1, 13)
    -- Set the mark before the cursor, should stay there
    curbufmeths.set_extmark(ns, marks[2], 1, 11)
    feed("i<cr><esc>")
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({2, 4}, rv)
    rv = curbufmeths.get_extmark(ns, marks[2])
    eq({1, 11}, rv)
    check_undo_redo(ns, marks[1], 1, 13, 2, 4)
  end)

  it('auto indenting entire line works #extmarks', function()
    feed(':set cindent<cr><esc>')
    feed(':set autoindent<cr><esc>')
    feed(':set shiftwidth=2<cr><esc>')
    -- <c-f> will force an indent of 2
    feed("0iint <esc>A {<cr><esc>0i1M1<esc>")
    curbufmeths.set_extmark(ns, marks[1], 2, 2)
    feed("0i<c-f><esc>")
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({2, 4}, rv)
    check_undo_redo(ns, marks[1], 2, 2, 2, 4)
    -- now check when cursor at eol
    feed("uA<c-f><esc>")
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({2, 4}, rv)
  end)

  it('removing auto indenting with <C-D> works #extmarks', function()
    feed(':set cindent<cr><esc>')
    feed(':set autoindent<cr><esc>')
    feed(':set shiftwidth=2<cr><esc>')
    feed("0i<tab><esc>")
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    feed("bi<c-d><esc>")
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({1, 2}, rv)
    check_undo_redo(ns, marks[1], 1, 4, 1, 2)
    -- check when cursor at eol
    feed("uA<c-d><esc>")
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({1, 2}, rv)
  end)

  it('indenting multiple lines with = works #extmarks', function()
    feed(':set cindent<cr><esc>')
    feed(':set autoindent<cr><esc>')
    feed(':set shiftwidth=2<cr><esc>')
    feed("0iint <esc>A {<cr><bs>1M1<cr><bs>2M2<esc>")
    curbufmeths.set_extmark(ns, marks[1], 2, 2)
    curbufmeths.set_extmark(ns, marks[2], 3, 2)
    feed('=gg')
    check_undo_redo(ns, marks[1], 2, 2, 2, 4)
    check_undo_redo(ns, marks[2], 3, 2, 3, 6)
  end)

  it('substitutes by deleting inside the replace matches #extmarks_sub', function()
    -- do_sub in ex_cmds.c
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    curbufmeths.set_extmark(ns, marks[2], 1, 4)
    feed(':s/34/xx<cr>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 5)
    check_undo_redo(ns, marks[2], 1, 4, 1, 5)
  end)

  it('substitutes when insert text > deleted #extmarks_sub', function()
    -- do_sub in ex_cmds.c
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    curbufmeths.set_extmark(ns, marks[2], 1, 4)
    feed(':s/34/xxx<cr>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 6)
    check_undo_redo(ns, marks[2], 1, 4, 1, 6)
  end)

  it('substitutes when marks around eol #extmarks_sub', function()
    -- do_sub in ex_cmds.c
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    curbufmeths.set_extmark(ns, marks[2], 1, 6)
    feed(':s/5/xxx<cr>')
    check_undo_redo(ns, marks[1], 1, 5, 1, 8)
    check_undo_redo(ns, marks[2], 1, 6, 1, 8)
  end)

  it('substitutes over range insert text > deleted #extmarks_sub', function()
    -- do_sub in ex_cmds.c
    feed('A<cr>x34xx<esc>')
    feed('A<cr>xxx34<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 3)
    curbufmeths.set_extmark(ns, marks[2], 2, 2)
    curbufmeths.set_extmark(ns, marks[3], 3, 5)
    feed(':1,3s/34/xxx<cr><esc>')
    check_undo_redo(ns, marks[1], 1, 3, 1, 6)
    check_undo_redo(ns, marks[2], 2, 2, 2, 5)
    check_undo_redo(ns, marks[3], 3, 5, 3, 7)
  end)

  it('substitutes multiple matches in a line #extmarks_sub', function()
    -- do_sub in ex_cmds.c
    feed('ddi3x3x3<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 1)
    curbufmeths.set_extmark(ns, marks[2], 1, 3)
    curbufmeths.set_extmark(ns, marks[3], 1, 5)
    feed(':s/3/yy/g<cr><esc>')
    check_undo_redo(ns, marks[1], 1, 1, 1, 3)
    check_undo_redo(ns, marks[2], 1, 3, 1, 6)
    check_undo_redo(ns, marks[3], 1, 5, 1, 9)
  end)

  it('substitions over multiple lines with newline in pattern #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    curbufmeths.set_extmark(ns, marks[2], 1, 5)
    curbufmeths.set_extmark(ns, marks[3], 2, 1)
    curbufmeths.set_extmark(ns, marks[4], 2, 6)
    curbufmeths.set_extmark(ns, marks[5], 3, 1)
    feed([[:1,2s:5\n:5 <cr>]])
    check_undo_redo(ns, marks[1], 1, 4, 1, 4)
    check_undo_redo(ns, marks[2], 1, 5, 1, 7)
    check_undo_redo(ns, marks[3], 2, 1, 1, 7)
    check_undo_redo(ns, marks[4], 2, 6, 1, 12)
    check_undo_redo(ns, marks[5], 3, 1, 2, 1)
  end)

  it('inserting #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    curbufmeths.set_extmark(ns, marks[2], 1, 5)
    curbufmeths.set_extmark(ns, marks[3], 2, 1)
    curbufmeths.set_extmark(ns, marks[4], 2, 6)
    curbufmeths.set_extmark(ns, marks[5], 3, 1)
    curbufmeths.set_extmark(ns, marks[6], 2, 3)
    feed([[:1,2s:5\n67:X<cr>]])
    check_undo_redo(ns, marks[1], 1, 4, 1, 4)
    check_undo_redo(ns, marks[2], 1, 5, 1, 6)
    check_undo_redo(ns, marks[3], 2, 1, 1, 6)
    check_undo_redo(ns, marks[4], 2, 6, 1, 9)
    check_undo_redo(ns, marks[5], 3, 1, 2, 1)
    check_undo_redo(ns, marks[6], 2, 3, 1, 6)
  end)

  it('substitions with multiple newlines in pattern #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    curbufmeths.set_extmark(ns, marks[2], 1, 6)
    curbufmeths.set_extmark(ns, marks[3], 2, 1)
    curbufmeths.set_extmark(ns, marks[4], 2, 6)
    curbufmeths.set_extmark(ns, marks[5], 3, 1)
    feed([[:1,2s:\n.*\n:X<cr>]])
    check_undo_redo(ns, marks[1], 1, 5, 1, 5)
    check_undo_redo(ns, marks[2], 1, 6, 1, 7)
    check_undo_redo(ns, marks[3], 2, 1, 1, 7)
    check_undo_redo(ns, marks[4], 2, 6, 1, 7)
    check_undo_redo(ns, marks[5], 3, 1, 1, 7)
  end)

  it('substitions over multiple lines with replace in substition #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 2)
    curbufmeths.set_extmark(ns, marks[2], 1, 3)
    curbufmeths.set_extmark(ns, marks[3], 1, 5)
    curbufmeths.set_extmark(ns, marks[4], 2, 1)
    curbufmeths.set_extmark(ns, marks[5], 3, 1)
    feed([[:1,2s:3:\r<cr>]])
    check_undo_redo(ns, marks[1], 1, 2, 1, 2)
    check_undo_redo(ns, marks[2], 1, 3, 2, 1)
    check_undo_redo(ns, marks[3], 1, 5, 2, 2)
    check_undo_redo(ns, marks[4], 2, 1, 3, 1)
    check_undo_redo(ns, marks[5], 3, 1, 4, 1)
    feed('u')
    feed([[:1,2s:3:\rxx<cr>]])
    eq({2, 4}, curbufmeths.get_extmark(ns, marks[3]))
  end)

  it('substitions over multiple lines with replace in substition #extmarks_sub', function()
    feed('A<cr>x3<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 2, 1)
    curbufmeths.set_extmark(ns, marks[2], 2, 2)
    curbufmeths.set_extmark(ns, marks[3], 2, 3)
    feed([[:2,2s:3:\r<cr>]])
    check_undo_redo(ns, marks[1], 2, 1, 2, 1)
    check_undo_redo(ns, marks[2], 2, 2, 3, 1)
    check_undo_redo(ns, marks[3], 2, 3, 3, 1)
  end)

  it('substitions over multiple lines with replace in substition #extmarks_sub', function()
    feed('A<cr>x3<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 2)
    curbufmeths.set_extmark(ns, marks[2], 1, 3)
    curbufmeths.set_extmark(ns, marks[3], 1, 5)
    curbufmeths.set_extmark(ns, marks[4], 2, 2)
    curbufmeths.set_extmark(ns, marks[5], 3, 1)
    feed([[:1,2s:3:\r<cr>]])
    check_undo_redo(ns, marks[1], 1, 2, 1, 2)
    check_undo_redo(ns, marks[2], 1, 3, 2, 1)
    check_undo_redo(ns, marks[3], 1, 5, 2, 2)
    check_undo_redo(ns, marks[4], 2, 2, 4, 1)
    check_undo_redo(ns, marks[5], 3, 1, 5, 1)
    feed('u')
    feed([[:1,2s:3:\rxx<cr>]])
    check_undo_redo(ns, marks[3], 1, 5, 2, 4)
  end)

  it('substitions with newline in match and sub, delta is 0 #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    curbufmeths.set_extmark(ns, marks[2], 1, 5)
    curbufmeths.set_extmark(ns, marks[3], 1, 6)
    curbufmeths.set_extmark(ns, marks[4], 2, 1)
    curbufmeths.set_extmark(ns, marks[5], 2, 6)
    curbufmeths.set_extmark(ns, marks[6], 3, 1)
    feed([[:1,1s:5\n:\r<cr>]])
    check_undo_redo(ns, marks[1], 1, 4, 1, 4)
    check_undo_redo(ns, marks[2], 1, 5, 2, 1)
    check_undo_redo(ns, marks[3], 1, 6, 2, 1)
    check_undo_redo(ns, marks[4], 2, 1, 2, 1)
    check_undo_redo(ns, marks[5], 2, 6, 2, 6)
    check_undo_redo(ns, marks[6], 3, 1, 3, 1)
  end)

  it('substitions with newline in match and sub, delta > 0 #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    curbufmeths.set_extmark(ns, marks[2], 1, 5)
    curbufmeths.set_extmark(ns, marks[3], 1, 6)
    curbufmeths.set_extmark(ns, marks[4], 2, 1)
    curbufmeths.set_extmark(ns, marks[5], 2, 6)
    curbufmeths.set_extmark(ns, marks[6], 3, 1)
    feed([[:1,1s:5\n:\r\r<cr>]])
    check_undo_redo(ns, marks[1], 1, 4, 1, 4)
    check_undo_redo(ns, marks[2], 1, 5, 3, 1)
    check_undo_redo(ns, marks[3], 1, 6, 3, 1)
    check_undo_redo(ns, marks[4], 2, 1, 3, 1)
    check_undo_redo(ns, marks[5], 2, 6, 3, 6)
    check_undo_redo(ns, marks[6], 3, 1, 4, 1)
  end)

  it('substitions with newline in match and sub, delta < 0 #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    curbufmeths.set_extmark(ns, marks[2], 1, 5)
    curbufmeths.set_extmark(ns, marks[3], 1, 6)
    curbufmeths.set_extmark(ns, marks[4], 2, 1)
    curbufmeths.set_extmark(ns, marks[5], 2, 6)
    curbufmeths.set_extmark(ns, marks[6], 3, 2)
    curbufmeths.set_extmark(ns, marks[7], 4, 1)
    feed([[:1,2s:5\n.*\n:\r<cr>]])
    check_undo_redo(ns, marks[1], 1, 4, 1, 4)
    check_undo_redo(ns, marks[2], 1, 5, 2, 1)
    check_undo_redo(ns, marks[3], 1, 6, 2, 1)
    check_undo_redo(ns, marks[4], 2, 1, 2, 1)
    check_undo_redo(ns, marks[5], 2, 6, 2, 1)
    check_undo_redo(ns, marks[6], 3, 2, 2, 2)
    check_undo_redo(ns, marks[7], 4, 1, 3, 1)
  end)

  it('substitions with backrefs, newline inserted into sub #extmarks_sub', function()
    feed('A<cr>67890<cr>xx<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 1, 4)
    curbufmeths.set_extmark(ns, marks[2], 1, 5)
    curbufmeths.set_extmark(ns, marks[3], 1, 6)
    curbufmeths.set_extmark(ns, marks[4], 2, 1)
    curbufmeths.set_extmark(ns, marks[5], 2, 6)
    curbufmeths.set_extmark(ns, marks[6], 3, 1)
    feed([[:1,1s:5\(\n\):\1\1<cr>]])
    check_undo_redo(ns, marks[1], 1, 4, 1, 4)
    check_undo_redo(ns, marks[2], 1, 5, 3, 1)
    check_undo_redo(ns, marks[3], 1, 6, 3, 1)
    check_undo_redo(ns, marks[4], 2, 1, 3, 1)
    check_undo_redo(ns, marks[5], 2, 6, 3, 6)
    check_undo_redo(ns, marks[6], 3, 1, 4, 1)
  end)

  it('using <c-a> when increase in order of magnitude #extmarks', function()
    -- do_addsub in ops.c
    feed('ddi9 abc<esc>H')
    -- when going from 9 to 10, mark on 9 shouldn't move
    curbufmeths.set_extmark(ns, marks[1], 1, 1)
    -- but things after the inserted number should be moved
    curbufmeths.set_extmark(ns, marks[2], 1, 3)
    feed('<c-a>')
    check_undo_redo(ns, marks[1], 1, 1, 1, 1)
    check_undo_redo(ns, marks[2], 1, 3, 1, 4)
  end)

  it('using <c-x> when decrease in order of magnitude #extmarks', function()
    -- do_addsub in ops.c
    feed('ddiab.10<esc>')
    -- when going from 10 to 9, mark on 0 should move
    curbufmeths.set_extmark(ns, marks[1], 1, 5)
    -- the mark before shouldn't move
    curbufmeths.set_extmark(ns, marks[2], 1, 3)
    feed('b<c-x>')
    rv = curbufmeths.get_extmark(ns, marks[2])
    eq({1, 3}, rv)
    check_undo_redo(ns, marks[1], 1, 5, 1, 4)
    -- TODO do we need to test marks[1] ???
    -- check_undo_redo(ns, marks[2], 1, 3, 1, 3)
  end)

  -- TODO catch exceptions
  pending('throws consistent error codes #todo', function()
    local ns_invalid = ns2 + 1
    rv = curbufmeths.set_extmark(ns_invalid, marks[1], positions[1][1], positions[1][2])
    rv = curbufmeths.del_extmark(ns_invalid, marks[1])
    rv = curbufmeths.list_extmarks(ns_invalid, positions[1], positions[2], ALL, 0)
    rv = curbufmeths.get_extmark(ns_invalid, marks[1])

  end)

  it('when col > line-length, set the mark on eol #extmarks', function()
    local invalid_col = init_text:len() + 1
    curbufmeths.set_extmark(ns, marks[1], 1, invalid_col)
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({1, init_text:len() + 1}, rv)
    -- Test another
    curbufmeths.set_extmark(ns, marks[1], 1, invalid_col)
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({1, init_text:len() + 1}, rv)
  end)

  it('when line > line, set the mark on end of buffer #extmarks', function()
    local invalid_col = init_text:len() + 1
    local invalid_lnum = 3 -- line1 ends in an eol. so line 2 contains a valid position (eol)?
    curbufmeths.set_extmark(ns, marks[1], invalid_lnum, invalid_col)
    rv = curbufmeths.get_extmark(ns, marks[1])
    eq({2, 1}, rv)
  end)

  it('bug from check_col in extmark_set #extmarks_sub', function()
    -- This bug was caused by extmark_set always using
    -- check_col. check_col always uses the current buffer.
    -- This wasn't working during undo so we now use
    -- check_col and check_lnum only when they are required.
    feed('A<cr>67890<cr>xx<esc>')
    feed('A<cr>12345<cr>67890<cr>xx<esc>')
    curbufmeths.set_extmark(ns, marks[1], 4, 5)
    feed([[:1,5s:5\n:5 <cr>]])
    check_undo_redo(ns, marks[1], 4, 5, 3, 7)
  end)

end)
