local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local clear, feed = helpers.clear, helpers.feed
local eval = helpers.eval
local eq = helpers.eq
local command = helpers.command

describe('ui/ext_messages', function()
  local screen

  before_each(function()
    clear()
    screen = Screen.new(25, 5)
    screen:attach({rgb=true, ext_messages=true, ext_popupmenu=true})
    screen:set_default_attr_ids({
      [1] = {bold = true, foreground = Screen.colors.Blue1},
      [2] = {foreground = Screen.colors.Grey100, background = Screen.colors.Red},
      [3] = {bold = true},
      [4] = {bold = true, foreground = Screen.colors.SeaGreen4},
    })
  end)

  it('supports :echoerr', function()
    feed(':echoerr "raa"<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
    ]], messages={{
      content = {{"raa", 2}},
      kind = "echoerr",
    }}}

    -- cmdline in a later input cycle clears error message
    feed(':')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
    ]], cmdline={{
      firstc = ":",
      content = {{ "" }},
      pos = 0,
    }}}


    feed('echoerr "bork" | echoerr "fail"<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
    ]], messages={{
        content = {{ "bork", 2 }},
        kind = "echoerr"
      }, {
        content = {{ "fail", 2 }},
        kind = "echoerr"
      }, {
        content = {{ "Press ENTER or type command to continue", 4 }},
        kind = "return_prompt"
    }}}

    feed(':echoerr "extrafail"<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
    ]], messages={{
        content = { { "bork", 2 } },
        kind = "echoerr"
      }, {
        content = { { "fail", 2 } },
        kind = "echoerr"
      }, {
        content = { { "extrafail", 2 } },
        kind = "echoerr"
      }, {
        content = { { "Press ENTER or type command to continue", 4 } },
        kind = "return_prompt"
    }}}

    feed('<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
    ]]}

    -- cmdline without interleaving wait/display keeps the error message
    feed(':echoerr "problem" | let x = input("foo> ")<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
    ]], messages={{
      content = {{ "problem", 2 }},
      kind = "echoerr"
    }}, cmdline={{
      prompt = "foo> ",
      content = {{ "" }},
      pos = 0,
    }}}

    feed('solution<cr>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
    ]]}
    eq('solution', eval('x'))
  end)

  it('supports showmode', function()
    command('imap <f2> <cmd>echomsg "stuff"<cr>')
    feed('i')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], showmode={{"-- INSERT --", 3}}}

    feed('<f2>')
    screen:expect{grid=[[
      ^                         |
      {1:~                        }|
      {1:~                        }|
      {1:~                        }|
                               |
    ]], messages={{
      content = { { "stuff" } },
      kind = "echomsg"
    }}, showmode={{ "-- INSERT --", 3 }}}


    feed('alphpabet<cr>alphanum<cr>')
    screen:expect{grid=[[
      alphpabet                |
      alphanum                 |
      ^                         |
      {1:~                        }|
      {1:~                        }|
    ]], messages={ {
        content = { { "stuff" } },
        kind = "echomsg"
      } }, showmode={ { "-- INSERT --", 3 } }}

    feed('<c-x>')
    screen:snapshot_util()
    
    feed('<c-p>')
    screen:snapshot_util()
  end)

end)

