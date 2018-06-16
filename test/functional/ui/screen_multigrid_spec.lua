local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local spawn, set_session, clear = helpers.spawn, helpers.set_session, helpers.clear
local feed, command = helpers.feed, helpers.command

describe('multigrid screen', function()
  local screen
  local nvim_argv = {helpers.nvim_prog, '-u', 'NONE', '-i', 'NONE', '-N',
                     '--cmd', 'set shortmess+=I background=light noswapfile belloff= noshowcmd noruler',
                     '--embed'}

  before_each(function()
    local screen_nvim = spawn(nvim_argv)
    set_session(screen_nvim)
    screen = Screen.new()
    screen:attach({ext_multigrid=true})
    screen:set_default_attr_ids({
      [1] = {bold = true, foreground = Screen.colors.Blue1},
      [2] = {foreground = Screen.colors.Magenta},
      [3] = {foreground = Screen.colors.Brown, bold = true},
      [4] = {foreground = Screen.colors.SlateBlue},
      [5] = {bold = true, foreground = Screen.colors.SlateBlue},
      [6] = {foreground = Screen.colors.Cyan4},
      [7] = {bold = true},
      [8] = {underline = true, bold = true, foreground = Screen.colors.SlateBlue},
      [9] = {foreground = Screen.colors.SlateBlue, underline = true},
      [10] = {foreground = Screen.colors.Red},
      [11] = {bold = true, reverse = true}
    })
  end)

  after_each(function()
    screen:detach()
  end)

  it('default initial screen', function()
    screen:expect([[
    ## grid 1
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
                                                           |
      {11:[No Name]                                            }|
                                                           |
    ## grid 2
      ^                                                     |
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
    ]])
  end)

end)
