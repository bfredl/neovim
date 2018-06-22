local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')

local clear, feed, insert = helpers.clear, helpers.feed, helpers.insert
local command, neq = helpers.command, helpers.neq
local curbufmeths = helpers.curbufmeths

describe('Buffer highlighting', function()
  local screen

  before_each(function()
    clear()
    command('syntax on')
    screen = Screen.new(40, 8)
    screen:attach({ext_hlstate=true})
    screen:set_default_attr_ids({
    })
  end)

  after_each(function()
    screen:detach()
  end)

  local add_hl = curbufmeths.add_highlight
  local clear_hl = curbufmeths.clear_highlight

  it('works', function()
    insert([[
      these are some lines
      with colorful text]])
    feed('+')

    add_hl(-1, "String", 0 , 10, 14)
    add_hl(-1, "Statement", 1 , 5, -1)

    screen:snapshot_util()
  end)

end)
