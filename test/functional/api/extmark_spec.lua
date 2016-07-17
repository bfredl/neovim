
local helpers = require('test.functional.helpers')(after_each)

local eq = helpers.eq
local buffer = helpers.buffer

describe('creating a new mark', function()
  before_each(helpers.clear)
  it('can create string marks that follow edits', function()
    local buf = helpers.nvim('get_current_buffer')
     -- eq(1, helpers.curbuf('line_count'))
    local string_mark = 'test'
    local text = 'some text to insert'
    local col = 0
    local row = 0

    local ret = buffer('mark_set', buf, string_mark, row, col)
    -- eq(ret, 1)
    helpers.insert(text)
    -- local pos = buffer('mark_index', buf, string_mark)
    -- eq(row, pos[1])
    -- eq(text.length(), pos[2])
  end)
end)

