local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local clear, feed = helpers.clear, helpers.feed
local source = helpers.source
local insert = helpers.insert
local meths = helpers.meths
local command = helpers.command
local funcs = helpers.funcs

describe('ui/ext_popupmenu', function()
  local screen
  before_each(function()
    clear()
    screen = Screen.new(60, 8)
    screen:attach({rgb=true, ext_popupmenu=true})
    screen:set_default_attr_ids({
      [1] = {bold=true, foreground=Screen.colors.Blue},
      [2] = {bold = true},
      [3] = {reverse = true},
      [4] = {bold = true, reverse = true},
      [5] = {bold = true, foreground = Screen.colors.SeaGreen},
      [6] = {background = Screen.colors.WebGray},
      [7] = {background = Screen.colors.LightMagenta},
    })
    source([[
      function! TestComplete() abort
        call complete(1, [{'word':'foo', 'abbr':'fo', 'menu':'the foo', 'info':'foo-y', 'kind':'x'}, 'bar', 'spam'])
        return ''
      endfunction
    ]])
  end)

  local expected = {
    {'fo', 'x', 'the foo', 'foo-y'},
    {'bar', '', '', ''},
    {'spam', '', '', ''},
  }

  it('works', function()
    feed('o<C-r>=TestComplete()<CR>')
    screen:expect{grid=[[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=0,
      anchor={1,0},
    }}

    feed('<c-p>')
    screen:expect{grid=[[
                                                                  |
      ^                                                            |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=-1,
      anchor={1,0},
    }}

    -- down moves the selection in the menu, but does not insert anything
    feed('<down><down>')
    screen:expect{grid=[[
                                                                  |
      ^                                                            |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=1,
      anchor={1,0},
    }}

    feed('<cr>')
    screen:expect{grid=[[
                                                                  |
      bar^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]]}
  end)

  it('can be controlled by API', function()
    feed('o<C-r>=TestComplete()<CR>')
    screen:expect{grid=[[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=0,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(1,false,false,{})
    screen:expect{grid=[[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=1,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(2,true,false,{})
    screen:expect{grid=[[
                                                                  |
      spam^                                                        |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=2,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(0,true,true,{})
    screen:expect([[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]])


    feed('<c-w><C-r>=TestComplete()<CR>')
    screen:expect{grid=[[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=0,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(-1,false,false,{})
    screen:expect{grid=[[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=-1,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(1,true,false,{})
    screen:expect{grid=[[
                                                                  |
      bar^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=1,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(-1,true,false,{})
    screen:expect{grid=[[
                                                                  |
      ^                                                            |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=-1,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(0,true,false,{})
    screen:expect{grid=[[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=0,
      anchor={1,0},
    }}

    meths.select_popupmenu_item(-1,true,true,{})
    screen:expect([[
                                                                  |
      ^                                                            |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]])

    command('imap <f1> <cmd>call nvim_select_popupmenu_item(2,v:true,v:false,{})<cr>')
    command('imap <f2> <cmd>call nvim_select_popupmenu_item(-1,v:false,v:false,{})<cr>')
    command('imap <f3> <cmd>call nvim_select_popupmenu_item(1,v:false,v:true,{})<cr>')
    feed('<C-r>=TestComplete()<CR>')
    screen:expect{grid=[[
                                                                  |
      foo^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=0,
      anchor={1,0},
    }}

    feed('<f1>')
    screen:expect{grid=[[
                                                                  |
      spam^                                                        |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=2,
      anchor={1,0},
    }}

    feed('<f2>')
    screen:expect{grid=[[
                                                                  |
      spam^                                                        |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]], popupmenu={
      items=expected,
      pos=-1,
      anchor={1,0},
    }}

    feed('<f3>')
    screen:expect([[
                                                                  |
      bar^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]])

    -- also should work for builtin popupmenu
    screen:set_option('ext_popupmenu', false)
    feed('<C-r>=TestComplete()<CR>')
    screen:expect([[
                                                                  |
      foo^                                                         |
      {6:fo   x the foo }{1:                                             }|
      {7:bar            }{1:                                             }|
      {7:spam           }{1:                                             }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]])

    feed('<f1>')
    screen:expect([[
                                                                  |
      spam^                                                        |
      {7:fo   x the foo }{1:                                             }|
      {7:bar            }{1:                                             }|
      {6:spam           }{1:                                             }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]])

    feed('<f2>')
    screen:expect([[
                                                                  |
      spam^                                                        |
      {7:fo   x the foo }{1:                                             }|
      {7:bar            }{1:                                             }|
      {7:spam           }{1:                                             }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]])

    feed('<f3>')
    screen:expect([[
                                                                  |
      bar^                                                         |
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {1:~                                                           }|
      {2:-- INSERT --}                                                |
    ]])
  end)
end)


describe('builtin popupmenu', function()
  local screen
  before_each(function()
    clear()
    screen = Screen.new(32, 20)
    screen:attach()
    screen:set_default_attr_ids({
      -- popup selected item / scrollbar track
      ['s'] = {background = Screen.colors.WebGray},
      -- popup non-selected item
      ['n'] = {background = Screen.colors.LightMagenta},
      -- popup scrollbar knob
      ['c'] = {background = Screen.colors.Grey0},
      [1] = {bold = true, foreground = Screen.colors.Blue},
      [2] = {bold = true},
      [3] = {reverse = true},
      [4] = {bold = true, reverse = true},
      [5] = {bold = true, foreground = Screen.colors.SeaGreen},
      [6] = {foreground = Screen.colors.Grey100, background = Screen.colors.Red},
    })
  end)

  it('works with preview-window above', function()
    feed(':ped<CR><c-w>4+')
    feed('iaa bb cc dd ee ff gg hh ii jj<cr>')
    feed('<c-x><c-n>')
    screen:expect([[
      aa bb cc dd ee ff gg hh ii jj   |
      aa                              |
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {3:[No Name] [Preview][+]          }|
      aa bb cc dd ee ff gg hh ii jj   |
      aa^                              |
      {s:aa             }{c: }{1:                }|
      {n:bb             }{c: }{1:                }|
      {n:cc             }{c: }{1:                }|
      {n:dd             }{c: }{1:                }|
      {n:ee             }{c: }{1:                }|
      {n:ff             }{c: }{1:                }|
      {n:gg             }{s: }{1:                }|
      {n:hh             }{s: }{4:                }|
      {2:-- }{5:match 1 of 10}                |
    ]])
  end)

  it('works with preview-window below', function()
    feed(':ped<CR><c-w>4+<c-w>r')
    feed('iaa bb cc dd ee ff gg hh ii jj<cr>')
    feed('<c-x><c-n>')
    screen:expect([[
      aa bb cc dd ee ff gg hh ii jj   |
      aa^                              |
      {s:aa             }{c: }{1:                }|
      {n:bb             }{c: }{1:                }|
      {n:cc             }{c: }{1:                }|
      {n:dd             }{c: }{1:                }|
      {n:ee             }{c: }{1:                }|
      {n:ff             }{c: }{1:                }|
      {n:gg             }{s: }{1:                }|
      {n:hh             }{s: }{4:                }|
      aa bb cc dd ee ff gg hh ii jj   |
      aa                              |
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {3:[No Name] [Preview][+]          }|
      {2:-- }{5:match 1 of 10}                |
      ]])
  end)

  it('works with preview-window above and tall and inverted', function()
    feed(':ped<CR><c-w>8+')
    feed('iaa<cr>bb<cr>cc<cr>dd<cr>ee<cr>')
    feed('ff<cr>gg<cr>hh<cr>ii<cr>jj<cr>')
    feed('kk<cr>ll<cr>mm<cr>nn<cr>oo<cr>')
    feed('<c-x><c-n>')
    screen:expect([[
      aa                              |
      bb                              |
      cc                              |
      dd                              |
      {s:aa             }{c: }{3:ew][+]          }|
      {n:bb             }{c: }                |
      {n:cc             }{c: }                |
      {n:dd             }{c: }                |
      {n:ee             }{c: }                |
      {n:ff             }{c: }                |
      {n:gg             }{c: }                |
      {n:hh             }{c: }                |
      {n:ii             }{c: }                |
      {n:jj             }{c: }                |
      {n:kk             }{c: }                |
      {n:ll             }{s: }                |
      {n:mm             }{s: }                |
      aa^                              |
      {4:[No Name] [+]                   }|
      {2:-- }{5:match 1 of 15}                |
    ]])
  end)

  it('works with preview-window above and short and inverted', function()
    feed(':ped<CR><c-w>4+')
    feed('iaa<cr>bb<cr>cc<cr>dd<cr>ee<cr>')
    feed('ff<cr>gg<cr>hh<cr>ii<cr>jj<cr>')
    feed('<c-x><c-n>')
    screen:expect([[
      aa                              |
      bb                              |
      cc                              |
      dd                              |
      ee                              |
      ff                              |
      gg                              |
      {s:aa             }                 |
      {n:bb             }{3:iew][+]          }|
      {n:cc             }                 |
      {n:dd             }                 |
      {n:ee             }                 |
      {n:ff             }                 |
      {n:gg             }                 |
      {n:hh             }                 |
      {n:ii             }                 |
      {n:jj             }                 |
      aa^                              |
      {4:[No Name] [+]                   }|
      {2:-- }{5:match 1 of 10}                |
    ]])
  end)

  it('works with preview-window below and inverted', function()
    feed(':ped<CR><c-w>4+<c-w>r')
    feed('iaa<cr>bb<cr>cc<cr>dd<cr>ee<cr>')
    feed('ff<cr>gg<cr>hh<cr>ii<cr>jj<cr>')
    feed('<c-x><c-n>')
    screen:expect([[
      {s:aa             }{c: }                |
      {n:bb             }{c: }                |
      {n:cc             }{c: }                |
      {n:dd             }{c: }                |
      {n:ee             }{c: }                |
      {n:ff             }{c: }                |
      {n:gg             }{s: }                |
      {n:hh             }{s: }                |
      aa^                              |
      {4:[No Name] [+]                   }|
      aa                              |
      bb                              |
      cc                              |
      dd                              |
      ee                              |
      ff                              |
      gg                              |
      hh                              |
      {3:[No Name] [Preview][+]          }|
      {2:-- }{5:match 1 of 10}                |
    ]])
  end)

  it('works with vsplits', function()
    insert('aaa aab aac\n')
    feed(':vsplit<cr>')
    screen:expect([[
      aaa aab aac         {3:│}aaa aab aac|
      ^                    {3:│}           |
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {4:[No Name] [+]        }{3:<Name] [+] }|
      :vsplit                         |
    ]])

    feed('ibbb a<c-x><c-n>')
    screen:expect([[
      aaa aab aac         {3:│}aaa aab aac|
      bbb aaa^             {3:│}bbb aaa    |
      {1:~  }{s: aaa            }{1: }{3:│}{1:~          }|
      {1:~  }{n: aab            }{1: }{3:│}{1:~          }|
      {1:~  }{n: aac            }{1: }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {1:~                   }{3:│}{1:~          }|
      {4:[No Name] [+]        }{3:<Name] [+] }|
      {2:-- }{5:match 1 of 3}                 |
    ]])

    feed('<esc><c-w><c-w>oc a<c-x><c-n>')
    screen:expect([[
      aaa aab aac{3:│}aaa aab aac         |
      bbb aaa    {3:│}bbb aaa             |
      c aaa      {3:│}c aaa^               |
      {1:~          }{3:│}{1:~}{s: aaa            }{1:   }|
      {1:~          }{3:│}{1:~}{n: aab            }{1:   }|
      {1:~          }{3:│}{1:~}{n: aac            }{1:   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {1:~          }{3:│}{1:~                   }|
      {3:<Name] [+]  }{4:[No Name] [+]       }|
      {2:-- }{5:match 1 of 3}                 |
    ]])
  end)

  it('works with split and scroll', function()
    screen:try_resize(60,14)
    command("split")
    command("set completeopt+=noinsert")
    command("set mouse=a")
    insert([[
      Lorem ipsum dolor sit amet, consectetur
      adipisicing elit, sed do eiusmod tempor
      incididunt ut labore et dolore magna aliqua.
      Ut enim ad minim veniam, quis nostrud
      exercitation ullamco laboris nisi ut aliquip ex
      ea commodo consequat. Duis aute irure dolor in
      reprehenderit in voluptate velit esse cillum
      dolore eu fugiat nulla pariatur. Excepteur sint
      occaecat cupidatat non proident, sunt in culpa
      qui officia deserunt mollit anim id est
      laborum.
    ]])

    screen:expect([[
        reprehenderit in voluptate velit esse cillum              |
        dolore eu fugiat nulla pariatur. Excepteur sint           |
        occaecat cupidatat non proident, sunt in culpa            |
        qui officia deserunt mollit anim id est                   |
        laborum.                                                  |
      ^                                                            |
      {4:[No Name] [+]                                               }|
        Lorem ipsum dolor sit amet, consectetur                   |
        adipisicing elit, sed do eiusmod tempor                   |
        incididunt ut labore et dolore magna aliqua.              |
        Ut enim ad minim veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
      {3:[No Name] [+]                                               }|
                                                                  |
    ]])

    feed('ggOEst <c-x><c-p>')
    screen:expect([[
      Est ^                                                        |
        L{n: sunt           }{s: }sit amet, consectetur                   |
        a{n: in             }{s: }sed do eiusmod tempor                   |
        i{n: culpa          }{s: }re et dolore magna aliqua.              |
        U{n: qui            }{s: }eniam, quis nostrud                     |
        e{n: officia        }{s: }co laboris nisi ut aliquip ex           |
      {4:[No}{n: deserunt       }{s: }{4:                                        }|
        L{n: mollit         }{s: }sit amet, consectetur                   |
        a{n: anim           }{s: }sed do eiusmod tempor                   |
        i{n: id             }{s: }re et dolore magna aliqua.              |
        U{n: est            }{s: }eniam, quis nostrud                     |
        e{n: laborum        }{c: }co laboris nisi ut aliquip ex           |
      {3:[No}{s: Est            }{c: }{3:                                        }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 65}            |
    ]])

    meths.input_mouse('wheel', 'down', '', 0, 9, 40)
    screen:expect([[
      Est ^                                                        |
        L{n: sunt           }{s: }sit amet, consectetur                   |
        a{n: in             }{s: }sed do eiusmod tempor                   |
        i{n: culpa          }{s: }re et dolore magna aliqua.              |
        U{n: qui            }{s: }eniam, quis nostrud                     |
        e{n: officia        }{s: }co laboris nisi ut aliquip ex           |
      {4:[No}{n: deserunt       }{s: }{4:                                        }|
        U{n: mollit         }{s: }eniam, quis nostrud                     |
        e{n: anim           }{s: }co laboris nisi ut aliquip ex           |
        e{n: id             }{s: }at. Duis aute irure dolor in            |
        r{n: est            }{s: }oluptate velit esse cillum              |
        d{n: laborum        }{c: }ulla pariatur. Excepteur sint           |
      {3:[No}{s: Est            }{c: }{3:                                        }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 65}            |
    ]])

    feed('e')
    screen:expect([[
      Est e^                                                       |
        L{n: elit           } sit amet, consectetur                   |
        a{n: eiusmod        } sed do eiusmod tempor                   |
        i{n: et             }ore et dolore magna aliqua.              |
        U{n: enim           }veniam, quis nostrud                     |
        e{n: exercitation   }mco laboris nisi ut aliquip ex           |
      {4:[No}{n: ex             }{4:                                         }|
        U{n: ea             }veniam, quis nostrud                     |
        e{n: esse           }mco laboris nisi ut aliquip ex           |
        e{n: eu             }uat. Duis aute irure dolor in            |
        r{s: est            }voluptate velit esse cillum              |
        dolore eu fugiat nulla pariatur. Excepteur sint           |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 65}            |
    ]])

    meths.input_mouse('wheel', 'up', '', 0, 9, 40)
    screen:expect([[
      Est e^                                                       |
        L{n: elit           } sit amet, consectetur                   |
        a{n: eiusmod        } sed do eiusmod tempor                   |
        i{n: et             }ore et dolore magna aliqua.              |
        U{n: enim           }veniam, quis nostrud                     |
        e{n: exercitation   }mco laboris nisi ut aliquip ex           |
      {4:[No}{n: ex             }{4:                                         }|
        L{n: ea             } sit amet, consectetur                   |
        a{n: esse           } sed do eiusmod tempor                   |
        i{n: eu             }ore et dolore magna aliqua.              |
        U{s: est            }veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 65}            |
    ]])

    feed('s')
    screen:expect([[
      Est es^                                                      |
        L{n: esse           } sit amet, consectetur                   |
        a{s: est            } sed do eiusmod tempor                   |
        incididunt ut labore et dolore magna aliqua.              |
        Ut enim ad minim veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
      {4:[No Name] [+]                                               }|
        Lorem ipsum dolor sit amet, consectetur                   |
        adipisicing elit, sed do eiusmod tempor                   |
        incididunt ut labore et dolore magna aliqua.              |
        Ut enim ad minim veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 65}            |
    ]])

    meths.input_mouse('wheel', 'down', '', 0, 9, 40)
    screen:expect([[
      Est es^                                                      |
        L{n: esse           } sit amet, consectetur                   |
        a{s: est            } sed do eiusmod tempor                   |
        incididunt ut labore et dolore magna aliqua.              |
        Ut enim ad minim veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
      {4:[No Name] [+]                                               }|
        Ut enim ad minim veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
        ea commodo consequat. Duis aute irure dolor in            |
        reprehenderit in voluptate velit esse cillum              |
        dolore eu fugiat nulla pariatur. Excepteur sint           |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 65}            |
    ]])

    feed('<bs>')
    screen:expect([[
      Est e^                                                       |
        L{n: elit           } sit amet, consectetur                   |
        a{n: eiusmod        } sed do eiusmod tempor                   |
        i{n: et             }ore et dolore magna aliqua.              |
        U{n: enim           }veniam, quis nostrud                     |
        e{n: exercitation   }mco laboris nisi ut aliquip ex           |
      {4:[No}{n: ex             }{4:                                         }|
        U{n: ea             }veniam, quis nostrud                     |
        e{n: esse           }mco laboris nisi ut aliquip ex           |
        e{n: eu             }uat. Duis aute irure dolor in            |
        r{s: est            }voluptate velit esse cillum              |
        dolore eu fugiat nulla pariatur. Excepteur sint           |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 65}            |
    ]])

    feed('<c-p>')
    screen:expect([[
      Est eu^                                                      |
        L{n: elit           } sit amet, consectetur                   |
        a{n: eiusmod        } sed do eiusmod tempor                   |
        i{n: et             }ore et dolore magna aliqua.              |
        U{n: enim           }veniam, quis nostrud                     |
        e{n: exercitation   }mco laboris nisi ut aliquip ex           |
      {4:[No}{n: ex             }{4:                                         }|
        U{n: ea             }veniam, quis nostrud                     |
        e{n: esse           }mco laboris nisi ut aliquip ex           |
        e{s: eu             }uat. Duis aute irure dolor in            |
        r{n: est            }voluptate velit esse cillum              |
        dolore eu fugiat nulla pariatur. Excepteur sint           |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 22 of 65}           |
    ]])

    meths.input_mouse('wheel', 'down', '', 0, 9, 40)
    screen:expect([[
      Est eu^                                                      |
        L{n: elit           } sit amet, consectetur                   |
        a{n: eiusmod        } sed do eiusmod tempor                   |
        i{n: et             }ore et dolore magna aliqua.              |
        U{n: enim           }veniam, quis nostrud                     |
        e{n: exercitation   }mco laboris nisi ut aliquip ex           |
      {4:[No}{n: ex             }{4:                                         }|
        r{n: ea             }voluptate velit esse cillum              |
        d{n: esse           }nulla pariatur. Excepteur sint           |
        o{s: eu             }t non proident, sunt in culpa            |
        q{n: est            }unt mollit anim id est                   |
        laborum.                                                  |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 22 of 65}           |
    ]])


    funcs.complete(4, {'ea', 'eeeeeeeeeeeeeeeeee', 'ei', 'eo', 'eu', 'ey', 'eå', 'eä', 'eö'})
    screen:expect([[
      Est eu^                                                      |
        {s: ea                 }t amet, consectetur                   |
        {n: eeeeeeeeeeeeeeeeee }d do eiusmod tempor                   |
        {n: ei                 } et dolore magna aliqua.              |
        {n: eo                 }iam, quis nostrud                     |
        {n: eu                 } laboris nisi ut aliquip ex           |
      {4:[N}{n: ey                 }{4:                                      }|
        {n: eå                 }uptate velit esse cillum              |
        {n: eä                 }la pariatur. Excepteur sint           |
        {n: eö                 }on proident, sunt in culpa            |
        qui officia deserunt mollit anim id est                   |
        laborum.                                                  |
      {3:[No Name] [+]                                               }|
      {2:-- Keyword Local completion (^N^P) }{5:match 1 of 9}             |
    ]])

    funcs.complete(4, {'ea', 'eee', 'ei', 'eo', 'eu', 'ey', 'eå', 'eä', 'eö'})
    screen:expect([[
      Est eu^                                                      |
        {s: ea             }r sit amet, consectetur                   |
        {n: eee            }, sed do eiusmod tempor                   |
        {n: ei             }bore et dolore magna aliqua.              |
        {n: eo             } veniam, quis nostrud                     |
        {n: eu             }amco laboris nisi ut aliquip ex           |
      {4:[N}{n: ey             }{4:                                          }|
        {n: eå             } voluptate velit esse cillum              |
        {n: eä             } nulla pariatur. Excepteur sint           |
        {n: eö             }at non proident, sunt in culpa            |
        qui officia deserunt mollit anim id est                   |
        laborum.                                                  |
      {3:[No Name] [+]                                               }|
      {2:-- INSERT --}                                                |
    ]])

    feed('<c-n>')
    screen:expect([[
      Esteee^                                                      |
        {n: ea             }r sit amet, consectetur                   |
        {s: eee            }, sed do eiusmod tempor                   |
        {n: ei             }bore et dolore magna aliqua.              |
        {n: eo             } veniam, quis nostrud                     |
        {n: eu             }amco laboris nisi ut aliquip ex           |
      {4:[N}{n: ey             }{4:                                          }|
        {n: eå             } voluptate velit esse cillum              |
        {n: eä             } nulla pariatur. Excepteur sint           |
        {n: eö             }at non proident, sunt in culpa            |
        qui officia deserunt mollit anim id est                   |
        laborum.                                                  |
      {3:[No Name] [+]                                               }|
      {2:-- INSERT --}                                                |
    ]])

    funcs.complete(6, {'foo', 'bar'})
    screen:expect([[
      Esteee^                                                      |
        Lo{s: foo            }sit amet, consectetur                   |
        ad{n: bar            }sed do eiusmod tempor                   |
        incididunt ut labore et dolore magna aliqua.              |
        Ut enim ad minim veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
      {4:[No Name] [+]                                               }|
        reprehenderit in voluptate velit esse cillum              |
        dolore eu fugiat nulla pariatur. Excepteur sint           |
        occaecat cupidatat non proident, sunt in culpa            |
        qui officia deserunt mollit anim id est                   |
        laborum.                                                  |
      {3:[No Name] [+]                                               }|
      {2:-- INSERT --}                                                |
    ]])

    feed('<c-y>')
    screen:expect([[
      Esteefoo^                                                    |
        Lorem ipsum dolor sit amet, consectetur                   |
        adipisicing elit, sed do eiusmod tempor                   |
        incididunt ut labore et dolore magna aliqua.              |
        Ut enim ad minim veniam, quis nostrud                     |
        exercitation ullamco laboris nisi ut aliquip ex           |
      {4:[No Name] [+]                                               }|
        reprehenderit in voluptate velit esse cillum              |
        dolore eu fugiat nulla pariatur. Excepteur sint           |
        occaecat cupidatat non proident, sunt in culpa            |
        qui officia deserunt mollit anim id est                   |
        laborum.                                                  |
      {3:[No Name] [+]                                               }|
      {2:-- INSERT --}                                                |
    ]])
  end)

  it('can be moved due to wrap or resize', function()
    feed('isome long prefix before the ')
    command("set completeopt+=noinsert,noselect")
    command("set linebreak")
    funcs.complete(29, {'word', 'choice', 'text', 'thing'})
    screen:expect([[
      some long prefix before the ^    |
      {1:~                        }{n: word  }|
      {1:~                        }{n: choice}|
      {1:~                        }{n: text  }|
      {1:~                        }{n: thing }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {2:-- INSERT --}                    |
    ]])

    feed('<c-p>')
    screen:expect([[
      some long prefix before the     |
      thing^                           |
      {n:word           }{1:                 }|
      {n:choice         }{1:                 }|
      {n:text           }{1:                 }|
      {s:thing          }{1:                 }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {2:-- INSERT --}                    |
    ]])

    feed('<c-p>')
    screen:expect([[
      some long prefix before the text|
      {1:^~                        }{n: word  }|
      {1:~                        }{n: choice}|
      {1:~                        }{s: text  }|
      {1:~                        }{n: thing }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {1:~                               }|
      {2:-- INSERT --}                    |
    ]])

    screen:try_resize(30,8)
    screen:expect([[
      some long prefix before the   |
      text^                          |
      {n:word           }{1:               }|
      {n:choice         }{1:               }|
      {s:text           }{1:               }|
      {n:thing          }{1:               }|
      {1:~                             }|
      {2:-- INSERT --}                  |
    ]])

    screen:try_resize(50,8)
    screen:expect([[
      some long prefix before the text^                  |
      {1:~                          }{n: word           }{1:       }|
      {1:~                          }{n: choice         }{1:       }|
      {1:~                          }{s: text           }{1:       }|
      {1:~                          }{n: thing          }{1:       }|
      {1:~                                                 }|
      {1:~                                                 }|
      {2:-- INSERT --}                                      |
    ]])
  end)

  it('can be moved due to wrap or resize', function()
    command("set rl")
    feed('isome rightleft ')
    screen:expect([[
                      ^  tfelthgir emos|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {2:-- INSERT --}                    |
    ]])

    command("set completeopt+=noinsert,noselect")
    funcs.complete(16, {'word', 'choice', 'text', 'thing'})
    screen:expect([[
                      ^  tfelthgir emos|
      {1:  }{n:           drow}{1:              ~}|
      {1:  }{n:         eciohc}{1:              ~}|
      {1:  }{n:           txet}{1:              ~}|
      {1:  }{n:          gniht}{1:              ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {2:-- INSERT --}                    |
    ]])

    feed('<c-n>')
    screen:expect([[
                  ^ drow tfelthgir emos|
      {1:  }{s:           drow}{1:              ~}|
      {1:  }{n:         eciohc}{1:              ~}|
      {1:  }{n:           txet}{1:              ~}|
      {1:  }{n:          gniht}{1:              ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {2:-- INSERT --}                    |
    ]])

    feed('<c-y>')
    screen:expect([[
                  ^ drow tfelthgir emos|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {1:                               ~}|
      {2:-- INSERT --}                    |
    ]])
  end)

  it('works with multiline messages', function()
    screen:try_resize(40,8)
    feed('ixx<cr>')
    command('imap <f2> <cmd>echoerr "very"\\|echoerr "much"\\|echoerr "error"<cr>')
    funcs.complete(1, {'word', 'choice', 'text', 'thing'})
    screen:expect([[
      xx                                      |
      word^                                    |
      {s:word           }{1:                         }|
      {n:choice         }{1:                         }|
      {n:text           }{1:                         }|
      {n:thing          }{1:                         }|
      {1:~                                       }|
      {2:-- INSERT --}                            |
    ]])

    feed('<f2>')
    screen:expect([[
      xx                                      |
      word                                    |
      {s:word           }{1:                         }|
      {4:                                        }|
      {6:very}                                    |
      {6:much}                                    |
      {6:error}                                   |
      {5:Press ENTER or type command to continue}^ |
    ]])

    feed('<cr>')
    screen:expect([[
      xx                                      |
      word^                                    |
      {s:word           }{1:                         }|
      {n:choice         }{1:                         }|
      {n:text           }{1:                         }|
      {n:thing          }{1:                         }|
      {1:~                                       }|
      {2:-- INSERT --}                            |
    ]])

    feed('<c-n>')
    screen:expect([[
      xx                                      |
      choice^                                  |
      {n:word           }{1:                         }|
      {s:choice         }{1:                         }|
      {n:text           }{1:                         }|
      {n:thing          }{1:                         }|
      {1:~                                       }|
      {2:-- INSERT --}                            |
    ]])

    command("split")
    screen:expect([[
      xx                                      |
      choice^                                  |
      {n:word           }{1:                         }|
      {s:choice         }{4:                         }|
      {n:text           }                         |
      {n:thing          }                         |
      {3:[No Name] [+]                           }|
      {2:-- INSERT --}                            |
    ]])

    meths.input_mouse('wheel', 'down', '', 0, 6, 15)
    screen:expect([[
      xx                                      |
      choice^                                  |
      {n:word           }{1:                         }|
      {s:choice         }{4:                         }|
      {n:text           }                         |
      {n:thing          }{1:                         }|
      {3:[No Name] [+]                           }|
      {2:-- INSERT --}                            |
    ]])
  end)


end)
