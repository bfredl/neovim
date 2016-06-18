-- Test the good behavior of the live action : substitution

local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local clear, feed, insert = helpers.clear, helpers.feed, helpers.insert
local execute = helpers.execute

describe('Live Substitution', function()
    local screen

    before_each(function()
        clear()
        execute("syntax on")
        execute("set livesub")
        execute('set nohlsearch')
        screen = Screen.new(40, 40)  -- 40 lines of 40 char
        screen:attach()
        screen:set_default_attr_ignore( {{bold=true, foreground=Screen.colors.Blue}} )
        screen:set_default_attr_ids({
            [1]  = {foreground = Screen.colors.Fuchsia},
            [2]  = {foreground = Screen.colors.Brown, bold = true},
            [3]  = {foreground = Screen.colors.SlateBlue},
            [4]  = {bold = true, foreground = Screen.colors.SlateBlue},
            [5]  = {foreground = Screen.colors.DarkCyan},
            [6]  = {bold = true},
            [7]  = {underline = true, bold = true, foreground = Screen.colors.SlateBlue},
            [8]  = {foreground = Screen.colors.Slateblue, underline = true},
            [9]  = {background = Screen.colors.Yellow},
            [10] = {reverse = true},
            [11] = {reverse = true, bold=true},
        })
    end)

    after_each(function()
        screen:detach()
    end)

    -- ----------------------------------------------------------------------
    -- simple tests
    -- ----------------------------------------------------------------------

    it('old behavior if :set nolivesub', function()
        insert([[
      these are some lines
      with colorful text (are)]])
        feed(':set nolivesub\n')
        feed(':%s/are/ARE')

        screen:expect([[
      these are some lines                    |
      with colorful text (are)                |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      :%s/are/ARE^                             |
    ]])

        feed('\27')     -- ESC
        feed(':%s/are/ARE\n')

        screen:expect([[
          these ARE some lines                    |
          ^with colorful text (ARE)                |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          :%s/are/ARE                             |
    ]])
    end)

    it('no split if :s', function()
        insert([[
      these are some lines
      with colorful text (are)]])
        feed(':s/are/ARE')

        screen:expect([[
      these are some lines                    |
      with colorful text (ARE)                |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      :s/are/ARE^                              |
    ]])
    end)

    it('split if :%s/are', function()
        insert([[
      these are some lines
      without colorful text (are)]])
        execute('set hlsearch')
        feed(':%s/are')
        screen:expect([[
          these {9:are} some lines                    |
          without colorful text ({9:are})             |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          {11:[No Name] [+]                           }|
           [1]these {9:are} some lines                |
           [2]without colorful text ({9:are})         |
                                                  |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          {10:[live_sub]                              }|
          :%s/are^                                 |
        ]])
    end)

    it('split if :%s/are/', function()
        insert([[
      these are some lines
      with colorful text (are)]])
        feed(':%s/are/')

        screen:expect([[
          these  some lines                       |
          with colorful text ()                   |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          {11:[No Name] [+]                           }|
           [1]these  some lines                   |
           [2]with colorful text ()               |
                                                  |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          {10:[live_sub]                              }|
          :%s/are/^                                |
        ]])
    end)


    it('split if :%s/are/to', function()
        insert([[
      these are some lines
      with colorful text (are)]])
        feed(':%s/are/to')

        screen:expect([[
          these to some lines                     |
          with colorful text (to)                 |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          {11:[No Name] [+]                           }|
           [1]these to some lines                 |
           [2]with colorful text (to)             |
                                                  |
          ~                                       |
          ~                                       |
          ~                                       |
          ~                                       |
          {10:[live_sub]                              }|
          :%s/are/to^                              |
        ]])
    end)

    -- ----------------------------------------------------------------------
    -- complex tests
    -- ----------------------------------------------------------------------

    it('scenario', function()
        insert([[
      these are some lines
      with colorful text (are)]])

        feed('gg')
        feed('2yy')
        feed('1000p')

        execute('set nohlsearch')

        execute(':%s/are/ARE')     -- simple sub

        screen:expect([[
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            these ARE some lines                    |
            with colorful text (ARE)                |
            ^with colorful text (ARE)                |
            2002 substitutions on 2002 lines        |
      ]])

        execute(':%s/some.*/nothing')      -- regex sub

        screen:expect([[
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        these ARE nothing                       |
        with colorful text (ARE)                |
        ^these ARE nothing                       |
        with colorful text (ARE)                |
        with colorful text (ARE)                |
        1001 substitutions on 1001 lines        |
       ]])

        feed('i')
        feed('example of insertion')

        screen:expect([[
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            these ARE nothing                       |
            with colorful text (ARE)                |
            example of insertion^these ARE nothing   |
            with colorful text (ARE)                |
            with colorful text (ARE)                |
            {6:-- INSERT --}                            |
       ]])

       feed('<esc>')
       execute('set livesub')
       execute('1,1000s/ARE/nut/g')
       feed(':%s/ARE/to')
       screen:expect([[
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         these to nothing                        |
         with colorful text (to)                 |
         example of insertionthese to nothing    |
         with colorful text (to)                 |
         with colorful text (to)                 |
         {11:[No Name] [+]                           }|
          [1001]with colorful text (to)          |
          [1002]these to nothing                 |
          [1003]with colorful text (to)          |
          [1004]these to nothing                 |
          [1005]with colorful text (to)          |
          [1006]these to nothing                 |
          [1007]with colorful text (to)          |
         {10:[live_sub]                              }|
         :%s/ARE/to^                              |
       ]])

        
    end)

end)
