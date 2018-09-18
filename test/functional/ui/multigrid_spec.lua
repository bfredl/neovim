local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local spawn, set_session, clear = helpers.spawn, helpers.set_session, helpers.clear
local feed, command = helpers.feed, helpers.command
local insert = helpers.insert
local eq, iswin = helpers.eq, helpers.iswin

-- Note 1:
-- Global grid i.e. "grid 1" shows some unwanted elements because they are
-- not cleared when they are expected to be drawn over by window grids.

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
      [11] = {bold = true, reverse = true},
      [12] = {reverse = true},
      [13] = {foreground = Screen.colors.DarkBlue, background = Screen.colors.LightGrey},
    })
    screen.win_position = {}
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

  it('positions windows correctly', function()
    command('vsplit')
    screen:expect([[
    ## grid 1
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
      {11:[No Name]                  }{12:[No Name]                 }|
                                                           |
    ## grid 2
                                |
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
    ## grid 3
      ^                          |
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
    ]], nil, nil, function()
      eq({
        [2] = { win = 1000, startrow = 0, startcol = 27, width = 26, height = 12 },
        [3] = { win = 1001, startrow = 0, startcol =  0, width = 26, height = 12 }
      }, screen.win_position)
    end)
    command('wincmd l')
    command('split')
    screen:expect([[
    ## grid 1
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}{11:[No Name]                 }|
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
      {12:[No Name]                  [No Name]                 }|
                                                           |
    ## grid 2
                                |
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
    ## grid 3
                                |
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
    ## grid 4
      ^                          |
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
      {1:~                         }|
    ]], nil, nil, function()
      eq({
        [2] = { win = 1000, startrow = 7, startcol = 27, width = 26, height =  5 },
        [3] = { win = 1001, startrow = 0, startcol =  0, width = 26, height = 12 },
        [4] = { win = 1002, startrow = 0, startcol = 27, width = 26, height =  6 }
      }, screen.win_position)
    end)
    command('wincmd h')
    command('q')
    screen:expect([[
    ## grid 1
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
      {11:[No Name]                                            }|
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
                                {12:│}                          |
      {12:[No Name]                                            }|
                                                           |
    ## grid 2
                                                           |
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
    ## grid 4
      ^                                                     |
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
      {1:~                                                    }|
    ]], nil, nil, function()
      eq({
        [2] = { win = 1000, startrow = 7, startcol = 0, width = 53, height =  5 },
        [4] = { win = 1002, startrow = 0, startcol = 0, width = 53, height =  6 }
      }, screen.win_position)
    end)
  end)

  describe('split', function ()
    describe('horizontally', function ()
      it('allocates grids', function ()
        command('sp')
        screen:expect([[
        ## grid 1
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
          {11:[No Name]                                            }|
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
          {12:[No Name]                                            }|
                                                               |
        ## grid 2
                                                               |
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
        ## grid 3
          ^                                                     |
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
        ]])
      end)

      it('resizes grids', function ()
        command('sp')
        command('resize 8')
        -- see "Note 1" for info about why there are three statuslines
        screen:expect([[
        ## grid 1
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
          {11:[No Name]                                            }|
                                                               |
          {11:[No Name]                                            }|
                                                               |
                                                               |
                                                               |
          {12:[No Name]                                            }|
                                                               |
        ## grid 2
                                                               |
          {1:~                                                    }|
          {1:~                                                    }|
        ## grid 3
          ^                                                     |
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
        ]])
      end)

      it('splits vertically', function()
        command('sp')
        command('vsp')
        command('vsp')
        screen:expect([[
        ## grid 1
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
          {11:[No Name]            }{12:[No Name]        [No Name]      }|
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
          {12:[No Name]                                            }|
                                                               |
        ## grid 2
                                                               |
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
        ## grid 3
                         |
          {1:~              }|
          {1:~              }|
          {1:~              }|
          {1:~              }|
          {1:~              }|
        ## grid 4
                          |
          {1:~               }|
          {1:~               }|
          {1:~               }|
          {1:~               }|
          {1:~               }|
        ## grid 5
          ^                    |
          {1:~                   }|
          {1:~                   }|
          {1:~                   }|
          {1:~                   }|
          {1:~                   }|
        ]])
        insert('hello')
        screen:expect([[
        ## grid 1
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
                              {12:│}                {12:│}               |
          {11:[No Name] [+]        }{12:[No Name] [+]    [No Name] [+]  }|
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
          {12:[No Name] [+]                                        }|
                                                               |
        ## grid 2
          hello                                                |
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
          {1:~                                                    }|
        ## grid 3
          hello          |
          {1:~              }|
          {1:~              }|
          {1:~              }|
          {1:~              }|
          {1:~              }|
        ## grid 4
          hello           |
          {1:~               }|
          {1:~               }|
          {1:~               }|
          {1:~               }|
          {1:~               }|
        ## grid 5
          hell^o               |
          {1:~                   }|
          {1:~                   }|
          {1:~                   }|
          {1:~                   }|
          {1:~                   }|
        ]])
      end)
      it('closes splits', function ()
        command('sp')
        command('q')
        screen:expect([[
        ## grid 1
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
                                                               |
          {11:[No Name]                                            }|
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

    describe('vertically', function ()
      it('allocates grids', function ()
        command('vsp')
        screen:expect([[
        ## grid 1
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
          {11:[No Name]                  }{12:[No Name]                 }|
                                                               |
        ## grid 2
                                    |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
        ## grid 3
          ^                          |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
        ]])
      end)
      it('resizes grids', function ()
        command('vsp')
        command('vertical resize 10')
        -- see "Note 1" for info about why there are two vseps
        screen:expect([[
        ## grid 1
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
                    {12:│}               {12:│}                          |
          {11:<No Name]  }{12:[No Name]                                 }|
                                                               |
        ## grid 2
                                                    |
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
          {1:~                                         }|
        ## grid 3
          ^          |
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
          {1:~         }|
        ]])
      end)
      it('splits horizontally', function ()
        command('vsp')
        command('sp')
        screen:expect([[
        ## grid 1
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
          {11:[No Name]                 }{12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
          {12:[No Name]                  [No Name]                 }|
                                                               |
        ## grid 2
                                    |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
        ## grid 3
                                    |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
        ## grid 4
          ^                          |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
        ]])
        insert('hello')
        screen:expect([[
        ## grid 1
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
          {11:[No Name] [+]             }{12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
          {12:[No Name] [+]              [No Name] [+]             }|
                                                               |
        ## grid 2
          hello                     |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
        ## grid 3
          hello                     |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
        ## grid 4
          hell^o                     |
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          {1:~                         }|
          ]])
      end)
      it('closes splits', function ()
        command('vsp')
        command('q')
        -- see "Note 1" for info about why there is a vsep
        screen:expect([[
        ## grid 1
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
                                    {12:│}                          |
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
  end)

  describe('on resize', function ()
    it('rebuilds all grids', function ()
      screen:try_resize(25, 6)
      screen:expect([[
      ## grid 1
                                 |
                                 |
                                 |
                                 |
        {11:[No Name]                }|
                                 |
      ## grid 2
        ^                         |
        {1:~                        }|
        {1:~                        }|
        {1:~                        }|
      ]])
    end)

    it('has minimum width/height values', function()
      screen:try_resize(1, 1)
      screen:expect([[
      ## grid 1
                    |
        {11:[No Name]   }|
                    |
      ## grid 2
        ^            |
      ]])

      feed('<esc>:ls')
      screen:expect([[
      ## grid 1
                    |
        {11:[No Name]   }|
        :ls^         |
      ## grid 2
                    |
      ]])
    end)
  end)

  describe('grid of smaller inner size', function()
    it('is rendered correctly', function()
      screen:try_resize_grid(2, 8, 5)
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
        ^        |
        {1:~       }|
        {1:~       }|
        {1:~       }|
        {1:~       }|
      ]])
    end)
  end)

  describe('grid of bigger inner size', function()
    it('is rendered correctly', function()
      screen:try_resize_grid(2, 80, 20)
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
        ^                                                                                |
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
        {1:~                                                                               }|
      ]])
    end)
  end)

  describe('with resized grid', function()
    before_each(function()
      screen:try_resize_grid(2, 60, 20)
    end)
    it('gets written till grid width', function()
      insert(('a'):rep(60).."\n")
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
        {11:[No Name] [+]                                        }|
                                                             |
      ## grid 2
        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa|
        ^                                                            |
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
      ]])
    end)

    it('wraps with grid width', function()
      insert(('b'):rep(80).."\n")
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
        {11:[No Name] [+]                                        }|
                                                             |
      ## grid 2
        bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb|
        bbbbbbbbbbbbbbbbbbbb                                        |
        ^                                                            |
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
      ]])
    end)

    it('displays messages with default grid width', function()
      command('echomsg "this is a very very very very very very very very'..
        ' long message"')
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
        this is a very very very...ry very very long message |
      ## grid 2
        ^                                                            |
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
      ]])
    end)

    it('creates folds with grid width', function()
      insert('this is a fold\nthis is inside fold\nthis is outside fold')
      feed('kzfgg')
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
        {11:[No Name] [+]                                        }|
                                                             |
      ## grid 2
        {13:^+--  2 lines: this is a fold································}|
        this is outside fold                                        |
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
        {1:~                                                           }|
      ]])
    end)
  end)
end)
