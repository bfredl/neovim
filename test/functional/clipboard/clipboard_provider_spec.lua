-- Test clipboard provider support

local helpers = require('test.functional.helpers')
local clear, feed, insert = helpers.clear, helpers.feed, helpers.insert
local execute, expect, eval = helpers.execute, helpers.expect, helpers.eval
local nvim, run, stop, restart = helpers.nvim, helpers.run, helpers.stop, helpers.restart

local function basic_register_test()
  insert("some words")

  feed('^dwP')
  expect('some words')

  feed('veyP')
  expect('some words words')

  feed('^dwywe"-p')
  expect('wordssome  words')

  feed('p')
  expect('wordssome words  words')

  feed('yyp')
  expect([[
      wordssome words  words
      wordssome words  words]])
  clear()
end

local clip = {
  ['+']={ {}, 'v' },
  ['*']={ {}, 'v' },
}

local function with_provider(test)
  local function on_request(method, args)
    if method == 'clipboard_get' then
      reg = args[1]
      print('get', reg, clip[reg][1][1])
      return clip[reg]
    elseif method == 'clipboard_set' then
      lines, regtype, reg = unpack(args)
      print('set', reg, lines[1])
      clip[reg] = {lines, regtype}
      return ''
    end
  end
  run(on_request,nil, function()
    nvim('register_provider', 'clipboard')
    test()
    stop()
  end)
  restart()
end

local function do_nothing()
    for i = 1,5 do
        nvim('get_buffers')
    end
end

describe('clipboard usage', function()
  setup(clear)
  it("works", function()
    basic_register_test()
    with_provider(function()
      basic_register_test()

      -- "* and unnamed should function as independent registes
      insert("some words")
      feed('^"*dwdw"*P')
      do_nothing()
      expect('some ')
      clear()

      -- "* and "+ should be independent when the provider supports it
      insert([[
        text:
        first line
        secound line
        third line]])

      feed('G"+dd"*dddd"+p"*pp')
      do_nothing()
      expect([[
        text:
        third line
        secound line
        first line]])

      if false then
          clear()
          execute('set clipboard=unnamed')

          -- the basic behavior of unnamed register should be the same
          -- even when handled by clipboard provider
          basic_register_test()

          -- with cb=unnamed, "* and unnamed will be the same register
          insert("some words")
          feed('^"*dwdw"*P')
          do_nothing()
          expect('words')
          clear()
      end
    end)

  end)
end)
