-- Test the good behavior of the live action : substitution

local helpers = require('test.functional.helpers')(after_each)
local Screen = require('test.functional.ui.screen')
local clear, feed, insert = helpers.clear, helpers.feed, helpers.insert
local execute, expect, eval = helpers.execute, helpers.expect, helpers.eval
local neq, source, eq = helpers.neq, helpers.source, helpers.eq

local default_text = [[
  Inc substitution on
  two lines
]]

local function common_setup(screen, incsub, text)
  if screen then
    execute("syntax on")
    execute("set nohlsearch")
    execute("hi IncSubstitute guifg=red guibg=yellow")
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
      [12] = {foreground = Screen.colors.Red, background = Screen.colors.Yellow},
      [13] = {bold = true, foreground = Screen.colors.SeaGreen},
      [14] = {foreground = Screen.colors.White, background = Screen.colors.Red},
    })
  end

  if incsub then
    execute("set incsubstitute="..incsub)
  else
    execute("set incsubstitute=")
  end

  if text then
    insert(text)
  end
end

describe('IncSubstitution preserves', function()
  before_each(clear)

  it(':ls functionality', function()
    local screen = Screen.new(30,10)
    common_setup(screen, "split", "ABC")

    execute("%s/AB/BA/")
    execute("ls")

    screen:expect([[
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      :ls                           |
        1 %a + "[No Name]"          |
                line 1              |
      {13:Press ENTER or type command to}|
      {13: continue}^                     |
    ]])
  end)

  it('default substitution with various delimiters', function()
    insert(default_text)
    execute("set incsubstitute=")

    local delims = { '/', '#', ';', '%', ',', '@', '!', ''}
    for _,delim in pairs(delims) do
      execute("%s"..delim.."lines"..delim.."LINES"..delim.."g")
      expect([[
        Inc substitution on
        two LINES
        ]])
      execute("undo")
    end
  end)

end)

describe('IncSubstitution preserves g+/g-', function()
  before_each(clear)
  local cases = { "", "split", "nosplit" }

  local substrings = {
    ":%s/1",
    ":%s/1/",
    ":%s/1/<bs>",
    ":%s/1/a",
    ":%s/1/a<bs>",
    ":%s/1/ax",
    ":%s/1/ax<bs>",
    ":%s/1/ax<bs><bs>",
    ":%s/1/ax<bs><bs><bs>",
    ":%s/1/ax/",
    ":%s/1/ax/<bs>",
    ":%s/1/ax/<bs>/",
    ":%s/1/ax/g",
    ":%s/1/ax/g<bs>",
    ":%s/1/ax/g<bs><bs>"
  }

  local function test_notsub(substring, split, redoable)
    clear()
    execute("set incsubstitute=" .. split)

    insert("1")
    feed("o2<esc>")
    execute("undo")
    feed("o3<esc>")
    if redoable then
      feed("o4<esc>")
      feed("u")
    end
    feed(substring .. "<esc>")

    feed("g-")
    expect([[
      1
      2]])

    feed("g+")
    expect([[
      1
      3]])

    if redoable then
      feed("<c-r>")
      expect([[
        1
        3
        4]])
    end
  end

  local function test_sub(substring, split, redoable)
      if redoable then
          pending("vim")
          return
      end
    clear()
    execute("set incsubstitute=" .. "")

    insert("1")
    feed("o2<esc>")
    execute("undo")
    feed("o3<esc>")
    if redoable then
      feed("o4<esc>")
      feed("u")
    end
    feed(substring.. "<enter>")
    feed("u")

    feed("g-")
    expect([[
      1
      2]])

    feed("g+")
    expect([[
      1
      3]])
  end

  for _, case in pairs(cases) do
    for _, redoable in pairs({true,false}) do
      for _, str in pairs(substrings) do
        it(", test_sub with "..str..", ics="..case..", redoable="..tostring(redoable),
           function()
            test_sub(str, case, redoable)
          end)

        it(", test_notsub with "..str..", ics="..case..", redoable="..tostring(redoable),
           function()
            test_notsub(str, case, redoable)
          end)
      end
    end
  end

end)


describe('IncSubstitution with incsubstitute=split', function()
  local screen

  before_each(function()
    clear()
    screen = Screen.new(30,15)
    common_setup(screen, "split", default_text .. default_text)
  end)

  after_each(function()
    if screen then screen:detach() end
  end)


  it('shows split window when typing the pattern', function()
    feed(":%s/tw")
    screen:expect([[
      Inc substitution on           |
      two lines                     |
                                    |
      ~                             |
      ~                             |
      {11:[No Name] [+]                 }|
       [2]two lines                 |
       [4]two lines                 |
                                    |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      {10:[inc_sub]                     }|
      :%s/tw^                        |
    ]])
  end)

  it('shows split window with empty replacement', function()
    feed(":%s/tw/")
    screen:expect([[
      Inc substitution on           |
      o lines                       |
                                    |
      ~                             |
      ~                             |
      {11:[No Name] [+]                 }|
       [2]o lines                   |
       [4]o lines                   |
                                    |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      {10:[inc_sub]                     }|
      :%s/tw/^                       |
    ]])

    feed("x<del>")
    screen:expect([[
      Inc substitution on           |
      o lines                       |
                                    |
      ~                             |
      ~                             |
      {11:[No Name] [+]                 }|
       [2]o lines                   |
       [4]o lines                   |
                                    |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      {10:[inc_sub]                     }|
      :%s/tw/^                       |
    ]])

  end)

  it('shows split window when typing replacement', function()
    feed(":%s/tw/XX")
    screen:expect([[
      Inc substitution on           |
      XXo lines                     |
                                    |
      ~                             |
      ~                             |
      {11:[No Name] [+]                 }|
       [2]{12:XX}o lines                 |
       [4]{12:XX}o lines                 |
                                    |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      {10:[inc_sub]                     }|
      :%s/tw/XX^                     |
    ]])
  end)

  it('does not show split window for :s/', function()
    feed("2gg")
    feed(":s/tw")
    screen:expect([[
      Inc substitution on           |
      two lines                     |
      Inc substitution on           |
      two lines                     |
                                    |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      :s/tw^                         |
    ]])
  end)

  it('highlights the pattern with :set hlsearch', function()
    execute("set hlsearch")
    feed(":%s/tw")
    screen:expect([[
      Inc substitution on           |
      {9:tw}o lines                     |
                                    |
      ~                             |
      ~                             |
      {11:[No Name] [+]                 }|
       [2]{9:tw}o lines                 |
       [4]{9:tw}o lines                 |
                                    |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      {10:[inc_sub]                     }|
      :%s/tw^                        |
    ]])
  end)

  it('actually replaces text', function()
    feed(":%s/tw/XX/g<enter>")

    screen:expect([[
      XXo lines                     |
      Inc substitution on           |
      ^XXo lines                     |
                                    |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      ~                             |
      :%s/tw/XX/g                   |
    ]])
  end)

  it('shows correct line numbers with many lines', function()
    feed("gg")
    feed("2yy")
    feed("1000p")
    execute("1,1000s/tw/BB/g")

    feed(":%s/tw/X")
    screen:expect([[
      Inc substitution on           |
      Xo lines                      |
      Xo lines                      |
      Inc substitution on           |
      Xo lines                      |
      {11:[No Name] [+]                 }|
       [1001]{12:X}o lines               |
       [1003]{12:X}o lines               |
       [1005]{12:X}o lines               |
       [1007]{12:X}o lines               |
       [1009]{12:X}o lines               |
       [1011]{12:X}o lines               |
       [1013]{12:X}o lines               |
      {10:[inc_sub]                     }|
      :%s/tw/X^                      |
    ]])
  end)

end)

describe('Incsubstitution with incsubstitute=nosplit', function()
  local screen

  before_each(function()
    clear()
    screen = Screen.new(20,10)
    common_setup(screen, "nosplit", default_text .. default_text)
  end)

  after_each(function()
    if screen then screen:detach() end
  end)

  it('does not show a split window anytime', function()
    execute("set hlsearch")

    feed(":%s/tw")
    screen:expect([[
      Inc substitution on |
      {9:tw}o lines           |
      Inc substitution on |
      {9:tw}o lines           |
                          |
      ~                   |
      ~                   |
      ~                   |
      ~                   |
      :%s/tw^              |
    ]])

    feed("/BM")
    screen:expect([[
      Inc substitution on |
      BMo lines           |
      Inc substitution on |
      BMo lines           |
                          |
      ~                   |
      ~                   |
      ~                   |
      ~                   |
      :%s/tw/BM^           |
    ]])

    feed("/")
    screen:expect([[
      Inc substitution on |
      BMo lines           |
      Inc substitution on |
      BMo lines           |
                          |
      ~                   |
      ~                   |
      ~                   |
      ~                   |
      :%s/tw/BM/^          |
    ]])

    feed("<enter>")
    screen:expect([[
      Inc substitution on |
      BMo lines           |
      Inc substitution on |
      ^BMo lines           |
                          |
      ~                   |
      ~                   |
      ~                   |
      ~                   |
      :%s/tw/BM/          |
    ]])

  end)

end)

describe('Incsubstitution with a failing expression', function()
  local screen = Screen.new(20,10)
  local cases = { "", "split", "nosplit" }

  for _, case in pairs(cases) do

    before_each(function()
      clear()
      common_setup(screen, case, default_text)
    end)

    it('in the pattern does nothing for, ics=' .. case, function()
      execute("set incsubstitute=" .. case)
      feed(":silent! %s/tw\\(/LARD/<enter>")
      expect(default_text)
    end)

    it('in the replacement deletes the matches, ics=' .. case, function()
      local replacements = { "\\='LARD", "\\=xx_novar__xx" }

      for _, repl in pairs(replacements) do
        execute("set incsubstitute=" .. case)
        feed(":silent! %s/tw/" .. repl .. "/<enter>")
        expect(string.gsub(default_text, "tw", ""))
        execute("undo")
      end
    end)

  end
end)

describe('Incsubstitution and cnoremap', function()
  local cases = { "",  "split", "nosplit" }

  for _, case in pairs(cases) do

    before_each(function()
      clear()
      common_setup(nil, case, default_text)
    end)

    it('work with remapped characters, ics=' .. case, function()
      local command = "%s/lines/LINES/g"

      for i = 1, string.len(command) do
        local c = string.sub(command, i, i)
        execute("cnoremap ".. c .. " " .. c)
      end

      execute(command)
      expect([[
        Inc substitution on
        two LINES
        ]])
    end)

    it('work then mappings move the cursor, ics=' .. case, function()
      execute("cnoremap ,S LINES/<left><left><left><left><left><left>")

      feed(":%s/lines/,Sor three <enter>")
      expect([[
        Inc substitution on
        two or three LINES
        ]])

      execute("cnoremap ;S /X/<left><left><left>")
      feed(":%s/;SI<enter>")
      expect([[
        Xnc substitution on
        two or three LXNES
        ]])

      execute("cnoremap ,T //Y/<left><left><left>")
      feed(":%s,TX<enter>")
      expect([[
        Ync substitution on
        two or three LYNES
        ]])

      execute("cnoremap ;T s//Z/<left><left><left>")
      feed(":%;TY<enter>")
      expect([[
        Znc substitution on
        two or three LZNES
        ]])
    end)

    it('work with a failing mapping, ics=' .. case, function()

      execute("cnoremap <expr> x capture('bwipeout!')[-1].'x'")

      feed(":%s/tw/tox<enter>")

      -- error thrown b/c of the mapping, substitution works anyways
      neq(nil, string.find(eval('v:errmsg'), '^E523:'))
      expect(string.gsub(default_text, "tw", "tox"))
    end)

    it('work when temporarily moving the cursor, ics=' .. case, function()
      execute("cnoremap <expr> x cursor(1, 1)[-1].'x'")

      feed(":%s/tw/tox/g<enter>")
      expect(string.gsub(default_text, "tw", "tox"))
    end)

    it('work when a mapping disables incsub, ics=' .. case, function()
      execute("cnoremap <expr> x capture('set incsubstitute=')[-1]")

      feed(":%s/tw/toxa/g<enter>")
      expect(string.gsub(default_text, "tw", "toa"))
    end)

    it('work with a complex mapping, ics=' .. case, function()
      source([[cnoremap x <C-\>eextend(g:, {'fo': getcmdline()})
      \.fo<CR><C-c>:new<CR>:bw!<CR>:<C-r>=remove(g:, 'fo')<CR>x]])

      feed(":%s/tw/tox")
      feed("/<enter>")
      expect(string.gsub(default_text, "tw", "tox"))
    end)

  end
end)

-- *** AUTOCMDS
-- Right now, none fire when opening or closing the incsub buffer
--
-- If that behavior is changed, add tests (suggestions by ZyX-l):
-- * If any are run when window is opened, then tests which check what if these
--   autocommands close the window.
-- * If any are run when window is opened, then
--   tests which check what if these autocommands wipe out buffer without closing
--   the window.
-- * If any are run when window is opened, then tests which check
--   what if these autocommands switch buffer without closing the window or
--   wiping buffer out.
-- * If any are run when window is opened, then tests which
--   check what if these autocommands wipe out buffer where substitution is
--   being made.
-- * If any are run when window is opened, then tests which check what
--   if these autocommands close window where substitution is being made (but
--   not window with temporary view)

describe('Incsubstitute: autocommands', function()
  before_each(clear)

  -- never, always, openOnly, closeOnly
  local eventsExpected = {
    ["BufAdd"] = "never",
    ["BufDelete"] = "never",
    ["BufEnter"] = "never",
    ["BufFilePost"] = "never",
    ["BufFilePre"] = "never",
    ["BufHidden"] = "never",
    ["BufLeave"] = "never",
    ["BufNew"] = "never",
    ["BufNewFile"] = "never",
    ["BufRead"] = "never",
    ["BufReadCmd"] = "never",
    ["BufReadPre"] = "never",
    ["BufUnload"] = "never",
    ["BufWinEnter"] = "never",
    ["BufWinLeave"] = "never",
    ["BufWipeout"] = "never",
    ["BufWrite"] = "never",
    ["BufWriteCmd"] = "never",
    ["BufWritePost"] = "never",
    ["Syntax"] = "never",
    ["FileType"] = "never",
    ["WinEnter"] = "never",
    ["WinLeave"] = "never",
    ["CmdwinEnter"] = "never",
    ["CmdwinLeave"] = "never",
  }

  local function register_autocmd(event)
    execute("let g:" .. event .. "_fired=0")
    execute("autocmd " .. event .. "* let g:" .. event .. "_fired=1")
  end

  local function aucmd_fired(event)
    return (eval("g:" .. event .. "_fired") == 1)
  end

  it('are run properly when splitting', function()
    common_setup(nil, "split", default_text)

    local eventsObserved = {}

    for event, _ in pairs(eventsExpected) do
      eventsObserved[event] = "never"
      register_autocmd(event)
    end

    feed(":%s/tw/KANU")

    for event, _ in pairs(eventsExpected) do
      if aucmd_fired(event) then
        eventsObserved[event] = "openOnly"
      end

      execute("let g:" .. event .. "_fired=0")
    end

    feed("/<enter>")

    for event, _ in pairs(eventsExpected) do
      if aucmd_fired(event) then
        -- never if not fired on open, else openOnly
        if eventsObserved[event] == "never" then
          eventsObserved[event] = "closeOnly"
        else
          eventsObserved[event] = "always"
        end
      end

      eq(event .. " " .. eventsExpected[event],
         event .. " " .. eventsObserved[event])
    end
  end)

end)

describe('Incsubstitute splits', function()
  local screen

  before_each(function()
    clear()
    screen = Screen.new(40,30)
    common_setup(screen, "split", default_text)
  end)

  after_each(function()
    screen:detach()
  end)

  it('work after other splittings', function()
    execute("vsplit")
    execute("split")

    feed(":%s/tw")
    screen:expect([[
      two lines           {10:|}two lines          |
                          {10:|}                   |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      {11:[No Name] [+]       }{10:|}~                  |
      two lines           {10:|}~                  |
                          {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      {10:[No Name] [+]        [No Name] [+]      }|
       [2]two lines                           |
                                              |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      {10:[inc_sub]                               }|
      :%s/tw^                                  |
    ]])

    feed("<esc>")
    execute("only")
    execute("split")
    execute("vsplit")

    feed(":%s/tw")
    screen:expect([[
      Inc substitution on {10:|}Inc substitution on|
      two lines           {10:|}two lines          |
                          {10:|}                   |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      ~                   {10:|}~                  |
      {11:[No Name] [+]        }{10:[No Name] [+]      }|
      Inc substitution on                     |
      two lines                               |
                                              |
      ~                                       |
      ~                                       |
      {10:[No Name] [+]                           }|
       [2]two lines                           |
                                              |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      ~                                       |
      {10:[inc_sub]                               }|
      :%s/tw^                                  |
    ]])
  end)

  local settings = {
  "splitbelow",
  "splitright",
  "noequalalways",
  "equalalways eadirection=ver",
  "equalalways eadirection=hor",
  "equalalways eadirection=both",
  }

  for _, setting in pairs(settings) do
    it("are not affected by " .. setting, function()

      execute("set " .. setting)

      feed(":%s/tw")
      screen:expect([[
        Inc substitution on                     |
        two lines                               |
                                                |
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
         [2]two lines                           |
                                                |
        ~                                       |
        ~                                       |
        ~                                       |
        ~                                       |
        ~                                       |
        {10:[inc_sub]                               }|
        :%s/tw^                                  |
      ]])
    end)
  end

end)
