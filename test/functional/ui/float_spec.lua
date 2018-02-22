local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local os = require('os')
local clear, feed, insert = helpers.clear, helpers.feed, helpers.insert
local command, feed_command = helpers.command, helpers.feed
local eval, exc_exec = helpers.eval, helpers.exc_exec
local feed_command, eq = helpers.feed_command, helpers.eq
local meths = helpers.meths
local curbufmeths = helpers.curbufmeths

describe('floating windows', function()
  before_each(function()
    clear()
  end)
  local attrs = {
    [0] = {bold=true, foreground=Screen.colors.Blue},
    [1] = {background = Screen.colors.LightMagenta},
    [2] = {background = Screen.colors.LightMagenta, bold = true, foreground = Screen.colors.Blue1},
    [3] = {bold = true},
    [4] = {bold = true, reverse = true},
    [5] = {reverse = true},
  }
  function with_multigrid(multigrid)
    before_each(function()
      info = {}
      screen = Screen.new(40,7)
      screen:attach({ext_multigrid=multigrid})
      screen:set_default_attr_ids(attrs)
      screen:set_on_event_handler(function(name, data)
        if name == "float_info" then
          local win, grid, width, height, options = unpack(data)
          -- TODO: fix this nvim side with conditional ui event
          -- if not multigrid then
          --   error("float_info unexpected")
          -- end
          if multigrid then
            info[grid] = {win, width, height, options}
          end
        elseif name == "float_close" then
          local win, grid  = unpack(data)
          info[grid] = nil
        end
      end)
    end)

    it('can be created and reconfigured', function()
      buf = meths.create_buf(false)
      win = meths.open_float_win(buf, false, 20, 2, {x=5, y=2})
      meths.win_set_option(win , 'winhl', 'Normal:PMenu')
      local expected_info = {[2]={
        {id=1001}, 20, 2, {
          anchor='NW',
          relative='editor',
          standalone=false,
          x=5,y=2,
        }
      }}


      if multigrid then
        screen:expect([[
        ## grid 1
          ^                                        |
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
                                                  |
        ## grid 2
          {1:                    }|
          {2:~                   }|
        ]], nil, nil, function()
          eq(expected_info, info)
        end)
      else
        screen:expect([[
          ^                                        |
          {0:~                                       }|
          {0:~    }{1:                    }{0:               }|
          {0:~    }{2:~                   }{0:               }|
          {0:~                                       }|
          {0:~                                       }|
                                                  |
          ]])
      end


      meths.win_config_float(win,0,0,{x=10, y=0})
      expected_info[2][4].x = 10
      expected_info[2][4].y = 0
      if multigrid then
        screen:expect([[
        ## grid 1
          ^                                        |
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
                                                  |
        ## grid 2
          {1:                    }|
          {2:~                   }|
        ]], nil, nil, function()
          eq(expected_info, info)
        end)
      else
        screen:expect([[
          ^          {1:                    }          |
          {0:~         }{2:~                   }{0:          }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
                                                  |
        ]])
      end
    end)

    if multigrid then
      it("supports second UI without multigrid", function()
        local session2 = helpers.connect(eval('v:servername'))
        print(session2:request("nvim_eval", "2+2"))
        local screen2 = Screen.new(40,7)
        screen2:attach(nil, session2)
        screen2:set_default_attr_ids(attrs)
        -- TODO: delet this:
        screen2:set_on_event_handler(function(name, data)
        end)
        buf = meths.create_buf(false)
        win = meths.open_float_win(buf, true, 20, 2, {x=5, y=2})
        meths.win_set_option(win, 'winhl', 'Normal:PMenu')
        local expected_info = {[2]={
          {id=1001}, 20, 2, {
            anchor='NW',
            relative='editor',
            standalone=false,
            x=5,y=2,
          }
        }}
        print("x")
        screen:expect([[
        ## grid 1
                                                  |
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
          {0:~                                       }|
                                                  |
        ## grid 2
          {1:^                    }|
          {2:~                   }|
        ]], nil, nil, function()
          eq(expected_info, info)
        end)
        screen2:expect([[
                                                  |
          {0:~                                       }|
          {0:~    }{1:^                    }{0:               }|
          {0:~    }{2:~                   }{0:               }|
          {0:~                                       }|
          {0:~                                       }|
                                                  |
          ]])
      end)
    end


    describe("handles :wincmd", function()
      local expected_info = {[2]={
        {id=1001}, 20, 2, {
          anchor='NW',
          relative='editor',
          standalone=false,
          x=5,y=2,
        }
      }}

      before_each(function()
        buf = meths.create_buf(false)
        win = meths.open_float_win(buf, false, 20, 2, {x=5, y=2})
        meths.win_set_option(win , 'winhl', 'Normal:PMenu')
        if multigrid then
          screen:expect([[
          ## grid 1
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
            ^                                        |
            {0:~                                       }|
            {0:~    }{1:                    }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end
      end)

      it("w", function()
        feed("<c-w>w")
        if multigrid then
          screen:expect([[
          ## grid 1
                                                    |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:^                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
                                                    |
            {0:~                                       }|
            {0:~    }{1:^                    }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end

        feed("<c-w>w")
        if multigrid then
          screen:expect([[
          ## grid 1
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
            ^                                        |
            {0:~                                       }|
            {0:~    }{1:                    }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end
      end)

      it("j", function()
        feed("<c-w>ji")
        if multigrid then
          screen:expect([[
          ## grid 1
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {3:-- INSERT --}                            |
          ## grid 2
            {1:                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
            ^                                        |
            {0:~                                       }|
            {0:~    }{1:                    }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
            {3:-- INSERT --}                            |
          ]])
        end
      end)

      it("s (non-float)", function()
        command("set laststatus=2")
        feed("<c-w>s")
        if multigrid then
          screen:expect([[
          ## grid 1
            ^                                        |
            {0:~                                       }|
            {4:[No Name]                               }|
                                                    |
            {0:~                                       }|
            {5:[No Name]                               }|
                                                    |
          ## grid 2
            {1:                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
            ^                                        |
            {0:~                                       }|
            {4:[No N}{1:                    }{4:               }|
                 {2:~                   }               |
            {0:~                                       }|
            {5:[No Name]                               }|
                                                    |
          ]])
        end

        feed("<c-w>w")
        if multigrid then
          screen:expect([[
          ## grid 1
                                                    |
            {0:~                                       }|
            {5:[No Name]                               }|
            ^                                        |
            {0:~                                       }|
            {4:[No Name]                               }|
                                                    |
          ## grid 2
            {1:                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
                                                    |
            {0:~                                       }|
            {5:[No N}{1:                    }{5:               }|
            ^     {2:~                   }               |
            {0:~                                       }|
            {4:[No Name]                               }|
                                                    |
          ]])
        end

        feed("<c-w>w")
        if multigrid then
          screen:expect([[
          ## grid 1
                                                    |
            {0:~                                       }|
            {5:[No Name]                               }|
                                                    |
            {0:~                                       }|
            {5:[No Name]                               }|
                                                    |
          ## grid 2
            {1:^                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
                                                    |
            {0:~                                       }|
            {5:[No N}{1:^                    }{5:               }|
                 {2:~                   }               |
            {0:~                                       }|
            {5:[No Name]                               }|
                                                    |
          ]])
        end


        feed("<c-w>w")
        if multigrid then
          screen:expect([[
          ## grid 1
            ^                                        |
            {0:~                                       }|
            {4:[No Name]                               }|
                                                    |
            {0:~                                       }|
            {5:[No Name]                               }|
                                                    |
          ## grid 2
            {1:                    }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, info)
          end)
        else
          screen:expect([[
            ^                                        |
            {0:~                                       }|
            {4:[No N}{1:                    }{4:               }|
                 {2:~                   }               |
            {0:~                                       }|
            {5:[No Name]                               }|
                                                    |
          ]])
        end
      end)
    end)
  end

  describe('with multigrid', function()
    with_multigrid(true)
  end)
  describe('without multigrid', function()
    with_multigrid(false)
  end)

end)

