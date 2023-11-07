-- Test for linebreak and list option (non-utf8)

local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local feed, insert, source = helpers.feed, helpers.insert, helpers.source
local clear, feed_command, expect = helpers.clear, helpers.feed_command, helpers.expect

describe('listlbr', function()
  before_each(clear)

  -- luacheck: ignore 621 (Indentation)
  -- luacheck: ignore 611 (Line contains only whitespaces)
  -- luacheck: ignore 613 (Trailing whitespaces in a string)
  it('is working #thetest', function()
    insert([[
      dummy text]])

    feed_command('set wildchar=^E')
    feed_command('10new')
    feed_command('vsp')
    feed_command('vert resize 20')
    feed_command([[put =\"\tabcdef hijklmn\tpqrstuvwxyz_1060ABCDEFGHIJKLMNOP \"]])
    feed_command('norm! zt')
    feed_command('set ts=4 sw=4 sts=4 linebreak sbr=+ wrap')

    local screen = Screen.new(80, 24)
    screen:set_default_attr_ids({
      [0] = {bold = true, foreground = Screen.colors.Blue},  -- NonText
      [1] = {background = Screen.colors.LightGrey},  -- Visual
      [2] = {background = Screen.colors.Red, foreground = Screen.colors.White},  -- ErrorMsg
      [3] = {bold = true, reverse = true};
      [4] = {reverse = true};
      [5] = {foreground = Screen.colors.Blue};
    })
    screen:attach()

    source([[
      fu! ScreenChar(width)
        let c=''
        for j in range(1,4)
          for i in range(1,a:width)
            let c.=nr2char(screenchar(j, i))
          endfor
          let c.="\n"
        endfor
        return c
      endfu
      fu! DoRecordScreen()
        wincmd l
        $put =printf(\"\n%s\", g:test)
        $put =g:line
        wincmd p
      endfu
    ]])
    feed_command('let g:test="Test 1: set linebreak"')
    feed_command('redraw!')
    feed_command('let line=ScreenChar(winwidth(0))')
    feed_command('call DoRecordScreen()')
    screen:expect{grid=[[
          ^abcdef          │                                                           |
      {0:+}hijklmn            │    abcdef hijklmn  pqrstuvwxyz_1060ABCDEFGHIJKLMNOP       |
      {0:+}pqrstuvwxyz_1060ABC│                                                           |
      {0:+}DEFGHIJKLMNOP      │Test 1: set linebreak                                      |
                          │    abcdef                                                 |
      Test 1: set         │+hijklmn                                                   |
      {0:+}linebreak          │+pqrstuvwxyz_1060ABC                                       |
          abcdef          │+DEFGHIJKLMNOP                                             |
      +hijklmn            │{0:~                                                          }|
      +pqrstuvwxyz_1060ABC│{0:~                                                          }|
      {3:[No Name] [+]        }{4:[No Name] [+]                                              }|
      dummy text                                                                      |
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {4:[No Name] [+]                                                                   }|
      4 more lines                                                                    |
    ]]}

    feed_command('let g:test="Test 2: set linebreak + set list"')
    feed_command('set linebreak list listchars=')
    feed_command('redraw!')
    feed_command('let line=ScreenChar(winwidth(0))')
    feed_command('call DoRecordScreen()')
    screen:expect{grid=[[
      {5:^I}^abcdef hijklmn{5:^I  }│    abcdef                                                 |
      {0:+}pqrstuvwxyz_1060ABC│+hijklmn                                                   |
      {0:+}DEFGHIJKLMNOP      │+pqrstuvwxyz_1060ABC                                       |
                          │+DEFGHIJKLMNOP                                             |
      Test 1: set         │                                                           |
      {0:+}linebreak          │Test 2: set linebreak + set list                           |
          abcdef          │^Iabcdef hijklmn^I                                         |
      +hijklmn            │+pqrstuvwxyz_1060ABC                                       |
      +pqrstuvwxyz_1060ABC│+DEFGHIJKLMNOP                                             |
      +DEFGHIJKLMNOP      │                                                           |
      {3:[No Name] [+]        }{4:[No Name] [+]                                              }|
      dummy text                                                                      |
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {4:[No Name] [+]                                                                   }|
      4 more lines                                                                    |
    ]]}

    feed_command('let g:test ="Test 3: set linebreak nolist"')
    feed_command('set nolist linebreak')
    feed_command('redraw!')
    feed_command('let line=ScreenChar(winwidth(0))')
    feed_command('call DoRecordScreen()')
    screen:expect{grid=[[
          ^abcdef          │^Iabcdef hijklmn^I                                         |
      {0:+}hijklmn            │+pqrstuvwxyz_1060ABC                                       |
      {0:+}pqrstuvwxyz_1060ABC│+DEFGHIJKLMNOP                                             |
      {0:+}DEFGHIJKLMNOP      │                                                           |
                          │                                                           |
      Test 1: set         │Test 3: set linebreak nolist                               |
      {0:+}linebreak          │    abcdef                                                 |
          abcdef          │+hijklmn                                                   |
      +hijklmn            │+pqrstuvwxyz_1060ABC                                       |
      +pqrstuvwxyz_1060ABC│+DEFGHIJKLMNOP                                             |
      {3:[No Name] [+]        }{4:[No Name] [+]                                              }|
      dummy text                                                                      |
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {4:[No Name] [+]                                                                   }|
      4 more lines                                                                    |
    ]]}

    feed_command('let g:test ="Test 4: set linebreak with tab and 1 line as long as screen: should break!"')
    feed_command('set nolist linebreak ts=8')
    feed_command([[let line="1\t".repeat('a', winwidth(0)-2)]])
    feed_command('$put =line')
    feed_command('$')
    feed_command('norm! zt')
    feed_command('redraw!')
    feed_command('let line=ScreenChar(winwidth(0))')
    feed_command('call DoRecordScreen()')
    screen:expect{grid=[[
      ^1                   │+pqrstuvwxyz_1060ABC                                       |
      {0:+}aaaaaaaaaaaaaaaaaa │+DEFGHIJKLMNOP                                             |
                          │1       aaaaaaaaaaaaaaaaaa                                 |
      Test 4: set         │                                                           |
      {0:+}linebreak with tab │Test 4: set linebreak with tab and 1 line as long as screen|
      {0:+}and 1 line as long │{0:+}: should break!                                           |
      {0:+}as screen: should  │1                                                          |
      {0:+}break!             │+aaaaaaaaaaaaaaaaaa                                        |
      1                   │~                                                          |
      +aaaaaaaaaaaaaaaaaa │~                                                          |
      {3:[No Name] [+]        }{4:[No Name] [+]                                              }|
      dummy text                                                                      |
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {4:[No Name] [+]                                                                   }|
      4 more lines                                                                    |
    ]]}

    feed_command([[let line="_S_\t bla"]])
    feed_command('$put =line')
    feed_command('$')
    feed_command('norm! zt')

    feed_command('let g:test ="Test 5: set linebreak with conceal and set list and tab displayed by different char (line may not be truncated)"')
    feed_command('set cpo&vim list linebreak conceallevel=2 concealcursor=nv listchars=tab:ab')
    feed_command('syn match ConcealVar contained /_/ conceal')
    feed_command('syn match All /.*/ contains=ConcealVar')
    feed_command('let line=ScreenChar(winwidth(0))')
    feed_command('call DoRecordScreen()')
    screen:expect{grid=[[
      ^S{0:abbbbbb} bla        │~                                                          |
                          │~                                                          |
      Test 5: set         │_S_      bla                                               |
      {0:+}linebreak with     │                                                           |
      {0:+}conceal and set    │Test 5: set linebreak with conceal and set list and tab dis|
      {0:+}list and tab       │{0:+}played by different char (line may not be truncated)      |
      {0:+}displayed by       │Sabbbbbb bla                                               |
      {0:+}different char     │~                                                          |
      {0:+}(line may not be   │~                                                          |
      {0:+}truncated)         │~                                                          |
      {3:[No Name] [+]        }{4:[No Name] [+]                                              }|
      dummy text                                                                      |
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {4:[No Name] [+]                                                                   }|
      4 more lines                                                                    |
    ]]}
    feed_command('set cpo&vim linebreak')

    feed_command('let g:test ="Test 6: set linebreak with visual block mode"')
    feed_command('let line="REMOVE: this not"')
    feed_command('$put =g:test')
    feed_command('$put =line')
    feed_command('let line="REMOVE: aaaaaaaaaaaaa"')
    feed_command('$put =line')
    feed_command('1/^REMOVE:')
    feed('0<C-V>jf x')
    feed_command('$put')
    feed_command('set cpo&vim linebreak')

    feed_command('let g:test ="Test 7: set linebreak with visual block mode and v_b_A"')
    feed_command('$put =g:test')
    feed('Golong line: <esc>40afoobar <esc>aTARGET at end<esc>')
    feed_command([[exe "norm! $3B\<C-v>eAx\<Esc>"]])
    feed_command('set cpo&vim linebreak sbr=')

    feed_command('let g:test ="Test 8: set linebreak with visual char mode and changing block"')
    feed_command('$put =g:test')
    feed('Go1111-1111-1111-11-1111-1111-1111<esc>0f-lv3lc2222<esc>bgj.')

    feed_command('let g:test ="Test 9: using redo after block visual mode"')
    feed_command('$put =g:test')
    feed('Go<CR>')
    feed('aaa<CR>')
    feed('aaa<CR>')
    feed('a<ESC>2k<C-V>2j~e.<CR>')

    feed_command('let g:test ="Test 10: using normal commands after block-visual"')
    feed_command('$put =g:test')
    feed_command('set linebreak')
    feed('Go<cr>')
    feed('abcd{ef<cr>')
    feed('ghijklm<cr>')
    feed('no}pqrs<esc>2k0f{<C-V><C-V>c%<esc>')

    feed_command('let g:test ="Test 11: using block replace mode after wrapping"')
    feed_command('$put =g:test')
    feed_command('set linebreak wrap')
    feed('Go<esc>150aa<esc>yypk147|<C-V>jr0<cr>')

    feed_command('let g:test ="Test 12: set linebreak list listchars=space:_,tab:>-,tail:-,eol:$"')
    feed_command('set list listchars=space:_,trail:-,tab:>-,eol:$')
    feed_command('$put =g:test')
    feed_command([[let line="a aaaaaaaaaaaaaaaaaaaaaa\ta "]])
    feed_command('$put =line')
    feed_command('$')
    feed_command('norm! zt')
    feed_command('redraw!')
    feed_command('let line=ScreenChar(winwidth(0))')
    feed_command('call DoRecordScreen()')
    screen:expect{grid=[[
      ^a{0:_}                  │Test 12: set linebreak list listchars=space:_,tab:>-,tail:-|
      aaaaaaaaaaaaaaaaaaaa│,eol:$                                                     |
      aa{0:>-----}a{0:-$}         │a aaaaaaaaaaaaaaaaaaaaaa        a                          |
      {0:$}                   │                                                           |
      Test{0:_}12:{0:_}set{0:_}       │Test 12: set linebreak list listchars=space:_,tab:>-,tail:-|
      linebreak{0:_}list{0:_}     │,eol:$                                                     |
      listchars=space:,   │a_                                                         |
      tab:>-,tail:-,eol:${0:$}│aaaaaaaaaaaaaaaaaaaa                                       |
      a{0:------------------} │aa>-----a-$                                                |
      {0:$}                   │~                                                          |
      {3:[No Name] [+]        }{4:[No Name] [+]                                              }|
      dummy text                                                                      |
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {0:~                                                                               }|
      {4:[No Name] [+]                                                                   }|
      4 more lines                                                                    |
    ]]}

    -- Assert buffer contents.
    expect([[

      	abcdef hijklmn	pqrstuvwxyz_1060ABCDEFGHIJKLMNOP 

      Test 1: set linebreak
          abcdef          
      +hijklmn            
      +pqrstuvwxyz_1060ABC
      +DEFGHIJKLMNOP      

      Test 2: set linebreak + set list
      ^Iabcdef hijklmn^I  
      +pqrstuvwxyz_1060ABC
      +DEFGHIJKLMNOP      
                          

      Test 3: set linebreak nolist
          abcdef          
      +hijklmn            
      +pqrstuvwxyz_1060ABC
      +DEFGHIJKLMNOP      
      1	aaaaaaaaaaaaaaaaaa

      Test 4: set linebreak with tab and 1 line as long as screen: should break!
      1                   
      +aaaaaaaaaaaaaaaaaa 
      ~                   
      ~                   
      _S_	 bla

      Test 5: set linebreak with conceal and set list and tab displayed by different char (line may not be truncated)
      Sabbbbbb bla        
      ~                   
      ~                   
      ~                   
      Test 6: set linebreak with visual block mode
      this not
      aaaaaaaaaaaaa
      REMOVE: 
      REMOVE: 
      Test 7: set linebreak with visual block mode and v_b_A
      long line: foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar foobar TARGETx at end
      Test 8: set linebreak with visual char mode and changing block
      1111-2222-1111-11-1111-2222-1111
      Test 9: using redo after block visual mode

      AaA
      AaA
      A
      Test 10: using normal commands after block-visual

      abcdpqrs
      Test 11: using block replace mode after wrapping
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0aaa
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0aaa
      Test 12: set linebreak list listchars=space:_,tab:>-,tail:-,eol:$
      a aaaaaaaaaaaaaaaaaaaaaa	a 

      Test 12: set linebreak list listchars=space:_,tab:>-,tail:-,eol:$
      a_                  
      aaaaaaaaaaaaaaaaaaaa
      aa>-----a-$         
      ~                   ]])
  end)

  -- oldtest: Test_linebreak_reset_restore()
  it('cursor position is drawn correctly after operator', function()
    local screen = Screen.new(60, 6)
    screen:set_default_attr_ids({
      [0] = {bold = true, foreground = Screen.colors.Blue},  -- NonText
      [1] = {background = Screen.colors.LightGrey},  -- Visual
      [2] = {background = Screen.colors.Red, foreground = Screen.colors.White},  -- ErrorMsg
    })
    screen:attach()

    -- f_wincol() calls validate_cursor()
    source([[
      set linebreak showcmd noshowmode formatexpr=wincol()-wincol()
      call setline(1, repeat('a', &columns - 10) .. ' bbbbbbbbbb c')
    ]])

    feed('$v$')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb {1:c}^                                                |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
                                                       2          |
    ]])
    feed('zo')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb ^c                                                |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
      {2:E490: No fold found}                                         |
    ]])

    feed('$v$')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb {1:c}^                                                |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
      {2:E490: No fold found}                              2          |
    ]])
    feed('gq')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb ^c                                                |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
      {2:E490: No fold found}                                         |
    ]])

    feed('$<C-V>$')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb {1:c}^                                                |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
      {2:E490: No fold found}                              1x2        |
    ]])
    feed('I')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb ^c                                                |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
      {2:E490: No fold found}                                         |
    ]])

    feed('<Esc>$v$')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb {1:c}^                                                |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
      {2:E490: No fold found}                              2          |
    ]])
    feed('s')
    screen:expect([[
      aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa          |
      bbbbbbbbbb ^                                                 |
      {0:~                                                           }|
      {0:~                                                           }|
      {0:~                                                           }|
      {2:E490: No fold found}                                         |
    ]])
  end)
end)
