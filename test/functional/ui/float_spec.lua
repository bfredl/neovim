local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local os = require('os')
local clear, feed = helpers.clear, helpers.feed
local command, feed_command = helpers.command, helpers.feed_command
local eval = helpers.eval
local eq = helpers.eq
local meths = helpers.meths
local curbufmeths = helpers.curbufmeths
local funcs = helpers.funcs
local run = helpers.run

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
    [6] = {background = Screen.colors.LightMagenta, bold = true, reverse = true},
    [7] = {foreground = Screen.colors.Grey100, background = Screen.colors.Red},
    [8] = {bold = true, foreground = Screen.colors.SeaGreen4},
    [9] = {background = Screen.colors.LightGrey, underline = true},
    [10] = {background = Screen.colors.LightGrey, underline = true, bold = true, foreground = Screen.colors.Magenta},
    [11] = {bold = true, foreground = Screen.colors.Magenta},
  }

  local function with_ext_float(ext_float)
    local screen
    before_each(function()
      screen = Screen.new(40,7)
      screen:attach({ext_float=ext_float})
      screen:set_default_attr_ids(attrs)
    end)

    it('can be created and reconfigured', function()
      local buf = meths.create_buf(false)
      local win = meths.open_float_win(buf, false, 20, 2, {x=5, y=2})
      meths.win_set_option(win , 'winhl', 'Normal:PMenu')
      local expected_info = {
          [2]={{id=1001}, 'NW', 1, 5, 2, {}},
      }


      if ext_float then
        screen:expect{grid=[[
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
        ]], condition=function()
          eq(expected_info, screen.float_info)
        end}
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
      expected_info[2][4] = 10
      expected_info[2][5] = 0
      if ext_float then
        screen:expect{grid=[[
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
        ]], condition=function()
          eq(expected_info, screen.float_info)
        end}
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

    if ext_float then
      pending("supports second UI without ext_float", function()
        local session2 = helpers.connect(eval('v:servername'))
        print(session2:request("nvim_eval", "2+2"))
        local screen2 = Screen.new(40,7)
        screen2:attach(nil, session2)
        screen2:set_default_attr_ids(attrs)
        local buf = meths.create_buf(false)
        local win = meths.open_float_win(buf, true, 20, 2, {x=5, y=2})
        meths.win_set_option(win, 'winhl', 'Normal:PMenu')
        local expected_info = {
          [2]={{id=1001}, 'NW', 1, 5, 2}
        }
        print("x")
        screen:expect{grid=[[
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
        ]], condition=function()
          eq(expected_info, screen.float_info)
        end}
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
      local win
      local expected_info
      before_each(function()
        -- the default, but be explicit:
        command("set laststatus=1")
        command("set hidden")
        meths.buf_set_lines(0,0,-1,true,{"x"})
        local buf = meths.create_buf(false)
        win = meths.open_float_win(buf, false, 20, 2, {x=5, y=2})
        meths.buf_set_lines(buf,0,-1,true,{"y"})
        meths.win_set_option(win , 'winhl', 'Normal:PMenu')
        local expected_info = {
          [2]={{id=1001}, 'NW', 1, 5, 2, {}}
        }
        if ext_float then
          screen:expect{grid=[[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
        ]], condition=function()
          eq(expected_info, screen.float_info)
        end}
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end
      end)

      it("w", function()
        feed("<c-w>w")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {0:~    }{1:^y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end

        feed("<c-w>w")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end
      end)

      it("W", function()
        feed("<c-w>W")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {0:~    }{1:^y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end

        feed("<c-w>W")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end
      end)

      it("j", function()
        feed("<c-w>ji") -- INSERT to trigger screen change
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {3:-- INSERT --}                            |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
            {3:-- INSERT --}                            |
          ]])
        end

        feed("<esc><c-w>w")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {0:~    }{1:^y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end

        feed("<c-w>j")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end

      end)

      it("s :split (non-float)", function()
        feed("<c-w>s")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {4:[No N}{1:y                   }{4:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ]])
        end

        feed("<c-w>w")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            ^x                                       |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {5:[No N}{1:y                   }{5:               }|
            ^x    {2:~                   }               |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
                                                    |
          ]])
        end

        feed("<c-w>w")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {5:[No N}{1:^y                   }{5:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ]])
        end


        feed("<c-w>w")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {4:[No N}{1:y                   }{4:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ]])
        end
      end)

      it("s :split (float)", function()
        feed("<c-w>w<c-w>s")
        if ext_float then
          screen:expect([[
          ## grid 1
            {1:^y                                       }|
            {2:~                                       }|
            {4:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            {1:^y                                       }|
            {2:~                                       }|
            {4:[No N}{1:y                   }{4:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ]])
        end

        feed(":set winhighlight=<cr><c-l>")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^y                                       |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^y                                       |
            {0:~                                       }|
            {4:[No N}{1:y                   }{4:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ]])
        end


        feed("<c-w>j")
        if ext_float then
          screen:expect([[
          ## grid 1
            y                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            ^x                                       |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            y                                       |
            {0:~                                       }|
            {5:[No N}{1:y                   }{5:               }|
            ^x    {2:~                   }               |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
                                                    |
          ]])
        end

        feed("<c-w>ji")
        if ext_float then
          screen:expect([[
          ## grid 1
            y                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            ^x                                       |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            {3:-- INSERT --}                            |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            y                                       |
            {0:~                                       }|
            {5:[No N}{1:y                   }{5:               }|
            ^x    {2:~                   }               |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            {3:-- INSERT --}                            |
          ]])
        end
      end)

      it(":new (non-float)", function()
        feed(":new<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^                                        |
            {0:~                                       }|
            {4:[No Name]                               }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            :new                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^                                        |
            {0:~                                       }|
            {4:[No N}{1:y                   }{4:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            :new                                    |
          ]])
        end
      end)

      it(":new (float)", function()
        feed("<c-w>w:new<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^                                        |
            {0:~                                       }|
            {4:[No Name]                               }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            :new                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^                                        |
            {0:~                                       }|
            {4:[No N}{1:y                   }{4:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            :new                                    |
          ]])
        end
      end)

      it("v :vsplit (non-float)", function()
        feed("<c-w>v")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                   {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {4:[No Name] [+]        }{5:[No Name] [+]      }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                   {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                   }{5:│}{0:~                  }|
            {4:[No Name] [+]        }{5:[No Name] [+]      }|
                                                    |
          ]])
        end
      end)

      it(":vnew (non-float)", function()
        feed(":vnew<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^                    {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {4:[No Name]            }{5:[No Name] [+]      }|
            :vnew                                   |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
        screen:expect([[
          ^                    {5:│}x                  |
          {0:~                   }{5:│}{0:~                  }|
          {0:~    }{1:y                   }{0:               }|
          {0:~    }{2:~                   }{0:               }|
          {0:~                   }{5:│}{0:~                  }|
          {4:[No Name]            }{5:[No Name] [+]      }|
          :vnew                                   |
        ]])
        end
      end)

      it(":vnew (float)", function()
        feed("<c-w>w:vnew<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^                    {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {0:~                   }{5:│}{0:~                  }|
            {4:[No Name]            }{5:[No Name] [+]      }|
            :vnew                                   |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^                    {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                   }{5:│}{0:~                  }|
            {4:[No Name]            }{5:[No Name] [+]      }|
            :vnew                                   |
          ]])
        end
      end)

      pending("exit by disconnect", function()
        -- FIXME: clear() is not enough, lua error must be triggered
        -- to detect ASAN failure. Maybe ignore this for now,
        -- as travis ASAN will detect this anyway...
        assert(false)
      end)


      it("q (:quit) last non-float exits nvim", function()
        command('autocmd VimLeave    * call rpcrequest(1, "exit")')
        -- avoid unsaved change in other buffer
        feed("<c-w><c-w>:w Xtest_written2<cr><c-w><c-w>")
        -- quit in last non-float
        feed(":wq Xtest_written<cr>")
        local exited = false
        local function on_request(name, args)
          eq("exit", name)
          eq({}, args)
          exited = true
          return 0
        end
        local function on_setup()
          feed(":wq Xtest_written<cr>")
        end
        run(on_request, nil, on_setup)
        os.remove('Xtest_written')
        os.remove('Xtest_written2')
        eq(exited, true)
      end)

      it("o (:only) non-float", function()
        feed("<c-w>o")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end
      end)

      it("o (:only) float fails", function()
        feed("<c-w>w<c-w>o")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            EXXX: floating win...not be only window |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {0:~    }{1:^y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
            EXXX: floating win...not be only window |
          ]])
        end
      end)

      it("o (:only) non-float with split", function()
        feed("<c-w>s")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {4:[No N}{1:y                   }{4:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ]])
        end

        feed("<c-w>o")
        if ext_float then
          screen:expect([[
          ## grid 1
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        else
          screen:expect([[
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
                                                    |
          ]])
        end
      end)

      it("o (:only) float with split", function()
        feed("<c-w>s<c-w>W")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {5:[No N}{1:^y                   }{5:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
                                                    |
          ]])
        end

        feed("<c-w>o")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            EXXX: floating win...not be only window |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {5:[No N}{1:^y                   }{5:               }|
            x    {2:~                   }               |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            EXXX: floating win...not be only window |
          ]])
        end
      end)

      it("J (float)", function()
        feed("<c-w>w<c-w>J")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {7:EXXX: cannot attach this float}          |
          ## grid 2
            {1:^y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {0:~    }{1:^y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
            {7:EXXX: cannot attach this float}          |
          ]])
        end

        meths.win_config_float(0,0,0,{standalone=true})
        feed(":<esc><c-w>J")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            {1:^y                                       }|
            {2:~                                       }|
            {4:[No Name] [+]                           }|
                                                    |
          ]])
        else
          screen:expect([[
            x                                       |
            {0:~                                       }|
            {5:[No Name] [+]                           }|
            {1:^y                                       }|
            {2:~                                       }|
            {4:[No Name] [+]                           }|
                                                    |
          ]])
        end

      end)

      it('movements with nested split layout', function()
        command("set hidden")
        feed("<c-w>s<c-w>v<c-w>b<c-w>v")
        if ext_float then
          screen:expect([[
          ## grid 1
            x                   {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {5:[No Name] [+]        [No Name] [+]      }|
            ^x                   {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {4:[No Name] [+]        }{5:[No Name] [+]      }|
                                                    |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            x                   {5:│}x                  |
            {0:~                   }{5:│}{0:~                  }|
            {5:[No N}{1:y                   }{5:Name] [+]      }|
            ^x    {2:~                   }               |
            {0:~                   }{5:│}{0:~                  }|
            {4:[No Name] [+]        }{5:[No Name] [+]      }|
                                                    |
          ]])
        end

        -- verify that N<c-w>w works
        for i = 1,5 do
          feed(i.."<c-w>w")
          feed_command("enew")
          curbufmeths.set_lines(0,-1,true,{tostring(i)})
        end

        if ext_float then
          screen:expect([[
          ## grid 1
            1                  {5:│}2                   |
            {0:~                  }{5:│}{0:~                   }|
            {5:[No Name] [+]       [No Name] [+]       }|
            3                  {5:│}4                   |
            {0:~                  }{5:│}{0:~                   }|
            {5:[No Name] [+]       [No Name] [+]       }|
            :enew                                   |
          ## grid 2
            ^5                   |
            {0:~                   }|
          ]])
        else
          screen:expect([[
            1                  {5:│}2                   |
            {0:~                  }{5:│}{0:~                   }|
            {5:[No N}^5                   {5:ame] [+]       }|
            3    {0:~                   }               |
            {0:~                  }{5:│}{0:~                   }|
            {5:[No Name] [+]       [No Name] [+]       }|
            :enew                                   |
          ]])
        end

        local movements = {
          w={2,3,4,5,1},
          W={5,1,2,3,4},
          h={1,1,3,3,3},
          j={3,3,3,4,4},
          k={1,2,1,1,1},
          l={2,2,4,4,4},
          t={1,1,1,1,1},
          b={4,4,4,4,4},
          p={4,4,4,3,3} -- TODO: not really proper, add dedicated test?
        }

        for k,v in pairs(movements) do
          for i = 1,5 do
            feed(i.."<c-w>w")
            feed('<c-w>'..k)
            local nr = funcs.winnr()
            eq(v[i],nr, "when using <c-w>"..k.." from window "..i)
          end
        end
      end)

      it(":tabnew and :tabnext", function()
        feed(":tabnew<cr>")
        if ext_float then
          -- grid is not freed, but float is marked as closed (should it rather be "invisible"?)
          screen:expect([[
          ## grid 1
            {9: }{10:2}{9:+ [No Name] }{3: [No Name] }{5:              }{9:X}|
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            :tabnew                                 |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]], nil, nil, function()
            eq({}, screen.float_info)
          end)
        else
          screen:expect([[
            {9: }{10:2}{9:+ [No Name] }{3: [No Name] }{5:              }{9:X}|
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            :tabnew                                 |
          ]])
        end

        feed(":tabnext<cr>")
        if ext_float then
            -- TODO: doesn't work
          screen:expect([[
          ## grid 1
            {3: }{11:2}{3:+ [No Name] }{9: [No Name] }{5:              }{9:X}|
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            :tabnext                                |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]], nil, nil, function()
            eq(expected_info, screen.float_info)
          end)
        else
          screen:expect([[
            {3: }{11:2}{3:+ [No Name] }{9: [No Name] }{5:              }{9:X}|
            ^x                                       |
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            :tabnext                                |
          ]])
        end

        feed(":tabnext<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            {9: }{10:2}{9:+ [No Name] }{3: [No Name] }{5:              }{9:X}|
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            :tabnext                                |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]], nil, nil, function()
            eq({}, screen.float_info)
          end)
        else
          screen:expect([[
            {9: }{10:2}{9:+ [No Name] }{3: [No Name] }{5:              }{9:X}|
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            :tabnext                                |
          ]])
        end
      end)

      it(":tabnew and :tabnext (standalone)", function()
        meths.win_config_float(win,0,0,{standalone=true})
        feed(":tabnew<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            {9: + [No Name] }{3: }{11:2}{3:+ [No Name] }{5:            }{9:X}|
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            :tabnew                                 |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            {9: + [No Name] }{3: }{11:2}{3:+ [No Name] }{5:            }{9:X}|
            ^                                        |
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {0:~                                       }|
            :tabnew                                 |
          ]])
        end

        feed(":tabnext<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            {3: }{11:2}{3:+ [No Name] }{9: [No Name] }{5:              }{9:X}|
            ^x                                       |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            :tabnext                                |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            {3: }{11:2}{3:+ [No Name] }{9: [No Name] }{5:              }{9:X}|
            ^x                                       |
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {4:[No Name] [+]                           }|
            :tabnext                                |
          ]])
        end

        feed(":tabnext<cr>")
        if ext_float then
          screen:expect([[
          ## grid 1
            {9: + [No Name] }{3: }{11:2}{3:+ [No Name] }{5:            }{9:X}|
            ^                                        |
            {0:~                                       }|
            {0:~                                       }|
            {0:~                                       }|
            {4:[No Name]                               }|
            :tabnext                                |
          ## grid 2
            {1:y                   }|
            {2:~                   }|
          ]])
        else
          screen:expect([[
            {9: + [No Name] }{3: }{11:2}{3:+ [No Name] }{5:            }{9:X}|
            ^                                        |
            {0:~    }{1:y                   }{0:               }|
            {0:~    }{2:~                   }{0:               }|
            {0:~                                       }|
            {4:[No Name]                               }|
            :tabnext                                |
          ]])
        end
      end)

      pending("templaty", function()
        feed("<c-w>")
        if ext_float then
          screen:snapshot_util()
        else
          screen:snapshot_util()
        end
      end)
    end)
  end

  describe('with ext_float', function()
    with_ext_float(true)
  end)
  describe('without ext_float', function()
    with_ext_float(false)
  end)

end)

