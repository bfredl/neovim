local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local spawn, set_session, clear = helpers.spawn, helpers.set_session, helpers.clear
local feed, command = helpers.feed, helpers.command
local insert = helpers.insert

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
      [12] = {reverse = true}
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
        command('vertical resize 8')
        -- see "Note 1" for info about why there are two vseps
        screen:expect([[
        ## grid 1
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
                  {12:│}                 {12:│}                          |
          {11:[No Name }{12:[No Name]                                   }|
                                                               |
        ## grid 2
                                                      |
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
          {1:~                                           }|
        ## grid 3
          ^        |
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
          {1:~       }|
        ]])
      end)
      it('splits horizontally', function ()
        command('vsp')
        command('sp')
        -- TODO
        insert('hello')
        -- TODO
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
                                                               |
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

end)
