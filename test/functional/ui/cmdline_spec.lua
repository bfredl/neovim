local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local clear, feed, eq = helpers.clear, helpers.feed, helpers.eq
local source = helpers.source
local ok = helpers.ok
local command = helpers.command

describe('external cmdline', function()
  local screen

  before_each(function()
    clear()
    screen = Screen.new(25, 5)
    screen:attach({rgb=true, ext_cmdline=true})
    screen:set_default_attr_ids({
      [1] = {bold = true, foreground = Screen.colors.Blue1},
      [2] = {reverse = true},
      [3] = {bold = true, reverse = true},
      [4] = {foreground = Screen.colors.Grey100, background = Screen.colors.Red},
      [5] = {bold = true, foreground = Screen.colors.SeaGreen4},
    })
  end)

  after_each(function()
    screen:detach()
  end)

  it('works', function()
    feed(':')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      firstc = ":",
      content = {{""}},
      pos = 0,
    }}}

    feed('sign')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      firstc = ":",
      content = {{"sign"}},
      pos = 4,
    }}}

    feed('<Left>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      firstc = ":",
      content = {{"sign"}},
      pos = 3,
    }}}

    feed('<bs>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      firstc = ":",
      content = {{"sin"}},
      pos = 2,
    }}}

    feed('<Esc>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]]}
  end)

  describe("redraws statusline on entering", function()
    before_each(function()
      command('set laststatus=2')
      command('set statusline=%{mode()}')
    end)

    it('from normal mode', function()
      screen:expect{grid=[[
        ^                         |
        {1:~                        }|
        {1:~                        }|
        {3:n                        }|
                                 |
      ]]}

      feed(':')
      screen:expect{grid=[[
        ^                         |
        {1:~                        }|
        {1:~                        }|
        {3:c                        }|
                                 |
      ]], cmdline={{
        firstc = ":",
        content = {{""}},
        pos = 0,
      }}}
    end)

    it('but not with scrolled messages', function()
      screen:try_resize(50,10)
      feed(':echoerr doesnotexist<cr>')
      screen:expect{grid=[[
                                                          |
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {3:                                                  }|
        {4:E121: Undefined variable: doesnotexist}            |
        {4:E15: Invalid expression: doesnotexist}             |
        {5:Press ENTER or type command to continue}^           |
      ]]}
      feed(':echoerr doesnotexist<cr>')
      screen:expect{grid=[[
                                                          |
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {3:                                                  }|
        {4:E121: Undefined variable: doesnotexist}            |
        {4:E15: Invalid expression: doesnotexist}             |
        {4:E121: Undefined variable: doesnotexist}            |
        {4:E15: Invalid expression: doesnotexist}             |
        {5:Press ENTER or type command to continue}^           |
      ]]}

      feed(':echoerr doesnotexist<cr>')
      screen:expect{grid=[[
                                                          |
        {1:~                                                 }|
        {3:                                                  }|
        {4:E121: Undefined variable: doesnotexist}            |
        {4:E15: Invalid expression: doesnotexist}             |
        {4:E121: Undefined variable: doesnotexist}            |
        {4:E15: Invalid expression: doesnotexist}             |
        {4:E121: Undefined variable: doesnotexist}            |
        {4:E15: Invalid expression: doesnotexist}             |
        {5:Press ENTER or type command to continue}^           |
      ]]}

      feed('<cr>')
      screen:expect{grid=[[
        ^                                                  |
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {1:~                                                 }|
        {3:n                                                 }|
                                                          |
      ]]}
    end)
  end)

  it("works with input()", function()
    feed(':call input("input", "default")<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      prompt = "input",
      content = {{"default"}},
      pos = 7,
    }}}

    feed('<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]]}
  end)

  it("works with special chars and nested cmdline", function()
    feed(':xx<c-r>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      firstc = ":",
      content = {{"xx"}},
      pos = 2,
      special = {'"', true},
    }}}

    feed('=')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      firstc = ":",
      content = {{"xx"}},
      pos = 2,
      special = {'"', true},
    }, {
      firstc = "=",
      content = {{""}},
      pos = 0,
    }}}

    feed('1+2')
    local expectation = {{
      firstc = ":",
      content = {{"xx"}},
      pos = 2,
      special = {'"', true},
    }, {
      firstc = "=",
      content = {{"1"}, {"+"}, {"2"}},
      pos = 3,
    }}

    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline=expectation}

    -- erase information, so we check if it is retransmitted
    -- TODO(bfredl): when we add a flag to screen:expect{}
    -- to explicitly check redraw!, it should also do this
    screen.cmdline = {}
    command("redraw!")
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline=expectation}


    feed('<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      firstc = ":",
      content = {{"xx3"}},
      pos = 3,
    }}}

    feed('<esc>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]]}
  end)

  it("works with function definitions", function()
    feed(':function Foo()<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      indent = 2,
      firstc = ":",
      content = {{""}},
      pos = 0,
    }}, cmdline_block = {
      {{'function Foo()'}},
    }}

    feed('line1<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      indent = 2,
      firstc = ":",
      content = {{""}},
      pos = 0,
    }}, cmdline_block = {
      {{'function Foo()'}},
      {{'  line1'}},
    }}

    screen.cmdline_block = {}
    command("redraw!")
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      indent = 2,
      firstc = ":",
      content = {{""}},
      pos = 0,
    }}, cmdline_block = {
      {{'function Foo()'}},
      {{'  line1'}},
    }}

    feed('endfunction<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]]}

    -- Try once more, to check buffer is reinitialized. #8007
    feed(':function Bar()<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], cmdline={{
      indent = 2,
      firstc = ":",
      content = {{""}},
      pos = 0,
    }}, cmdline_block = {
      {{'function Bar()'}},
    }}

    feed('endfunction<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]]}

  end)

  it("works with cmdline window", function()
    feed(':make')
    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "make" } },
        firstc = ":",
        indent = 0,
        pos = 4,
        prompt = ""
      }}, screen.cmdline)
    end)

    feed('<c-f>')
    screen:expect([[
                               |
      {2:[No Name]                }|
      {1::}make^                    |
      {3:[Command Line]           }|
                               |
    ]], nil, nil, function()
      eq({}, screen.cmdline)
    end)

    -- nested cmdline
    feed(':yank')
    screen:expect([[
                               |
      {2:[No Name]                }|
      {1::}make^                    |
      {3:[Command Line]           }|
                               |
    ]], nil, nil, function()
      eq({nil, {
        content = { { {}, "yank" } },
        firstc = ":",
        indent = 0,
        pos = 4,
        prompt = ""
      }}, screen.cmdline)
    end)

    screen.cmdline = {}
    command("redraw!")
    screen:expect([[
                               |
      {2:[No Name]                }|
      {1::}make^                    |
      {3:[Command Line]           }|
                               |
    ]], nil, nil, function()
      eq({nil, {
        content = { { {}, "yank" } },
        firstc = ":",
        indent = 0,
        pos = 4,
        prompt = ""
      }}, screen.cmdline)
    end)

    feed("<c-c>")
    screen:expect([[
                               |
      {2:[No Name]                }|
      {1::}make^                    |
      {3:[Command Line]           }|
                               |
    ]], nil, nil, function()
      eq({}, screen.cmdline)
    end)

    feed("<c-c>")
    screen:expect([[
                               |
      {2:[No Name]                }|
      {1::}make^                    |
      {3:[Command Line]           }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "make" } },
        firstc = ":",
        indent = 0,
        pos = 4,
        prompt = ""
      }}, screen.cmdline)
    end)

    screen.cmdline = {}
    command("redraw!")
    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "make" } },
        firstc = ":",
        indent = 0,
        pos = 4,
        prompt = ""
      }}, screen.cmdline)
    end)
  end)

  it('works with inputsecret()', function()
    feed(":call inputsecret('secret:')<cr>abc123")
    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "******" } },
        firstc = "",
        indent = 0,
        pos = 6,
        prompt = "secret:"
      }}, screen.cmdline)
    end)
  end)

  it('works with highlighted cmdline #thetest', function()
    source([[
      highlight RBP1 guibg=Red
      highlight RBP2 guibg=Yellow
      highlight RBP3 guibg=Green
      highlight RBP4 guibg=Blue
      let g:NUM_LVLS = 4
      function RainBowParens(cmdline)
        let ret = []
        let i = 0
        let lvl = 0
        while i < len(a:cmdline)
          if a:cmdline[i] is# '('
            call add(ret, [i, i + 1, 'RBP' . ((lvl % g:NUM_LVLS) + 1)])
            let lvl += 1
          elseif a:cmdline[i] is# ')'
            let lvl -= 1
            call add(ret, [i, i + 1, 'RBP' . ((lvl % g:NUM_LVLS) + 1)])
          endif
          let i += 1
        endwhile
        return ret
      endfunction
      map <f5>  :let x = input({'prompt':'>','highlight':'RainBowParens'})<cr>
      "map <f5>  :let x = input({'prompt':'>'})<cr>
    ]])
    screen:set_default_attr_ids({
      RBP1={background = Screen.colors.Red},
      RBP2={background = Screen.colors.Yellow},
      RBP3={background = Screen.colors.Green},
      RBP4={background = Screen.colors.Blue},
      EOB={bold = true, foreground = Screen.colors.Blue1},
      ERR={foreground = Screen.colors.Grey100, background = Screen.colors.Red},
      SK={foreground = Screen.colors.Blue},
      PE={bold = true, foreground = Screen.colors.SeaGreen4}
    })
    feed('<f5>(a(b)a)')
    screen:sleep(50)
    screen:expect{grid=[[
      ^                         |
      {EOB:~                        }|
      {EOB:~                        }|
      {EOB:~                        }|
                               |
    ]], cmdline={{
      prompt = '>',
      content = {{'(e', 'RBP1'}, {'a'}, {'(', 'RBP2'}, {'b'},
                 { ')', 'RBP2'}, {'a'}, {')', 'RBP1'}},
      pos = 7,
    }}}
  end)

  it('works together with ext_wildmenu', function()
    local expected = {
      'define',
      'jump',
      'list',
      'place',
      'undefine',
      'unplace',
    }

    command('set wildmode=full')
    command('set wildmenu')
    screen:set_option('ext_wildmenu', true)
    feed(':sign <tab>')

    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "sign define"} },
        firstc = ":",
        indent = 0,
        pos = 11,
        prompt = ""
      }}, screen.cmdline)
      eq(expected, screen.wildmenu_items)
      eq(0, screen.wildmenu_pos)
    end)

    feed('<tab>')
    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "sign jump"} },
        firstc = ":",
        indent = 0,
        pos = 9,
        prompt = ""
      }}, screen.cmdline)
      eq(expected, screen.wildmenu_items)
      eq(1, screen.wildmenu_pos)
    end)

    feed('<left><left>')
    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "sign "} },
        firstc = ":",
        indent = 0,
        pos = 5,
        prompt = ""
      }}, screen.cmdline)
      eq(expected, screen.wildmenu_items)
      eq(-1, screen.wildmenu_pos)
    end)

    feed('<right>')
    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "sign define"} },
        firstc = ":",
        indent = 0,
        pos = 11,
        prompt = ""
      }}, screen.cmdline)
      eq(expected, screen.wildmenu_items)
      eq(0, screen.wildmenu_pos)
    end)

    feed('a')
    screen:expect([[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], nil, nil, function()
      eq({{
        content = { { {}, "sign definea"} },
        firstc = ":",
        indent = 0,
        pos = 12,
        prompt = ""
      }}, screen.cmdline)
      eq(nil, screen.wildmenu_items)
      eq(nil, screen.wildmenu_pos)
    end)
  end)
end)
