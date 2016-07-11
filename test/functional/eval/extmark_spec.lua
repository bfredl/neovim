
local helpers = require('test.functional.helpers')(after_each)
local clear, nvim, buffer = helpers.clear, helpers.nvim, helpers.buffer
local curbuf, curwin, eq = helpers.curbuf, helpers.curwin, helpers.eq
local curbufmeths, ok = helpers.curbufmeths, helpers.ok
local funcs = helpers.funcs

describe('arbitrary marks set', function()
  it('works', function()
      curbuf('insert', -1, {'a', 'bit of', 'text'})
      nvim('command', 'arb_mark haha 0 0')
      curwin('set_cursor', {0, 0})
      curbuf('insert', -1, {'12345'})
      pos = nvim('eval', 'arbmark_index(haha)')
      eq(pos, {0, 6})
    end)
  end)

