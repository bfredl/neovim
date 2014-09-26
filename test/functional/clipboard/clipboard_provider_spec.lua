-- Test clipboard provider support

local helpers = require('test.functional.helpers')
local clear, feed, insert = helpers.clear, helpers.feed, helpers.insert
local execute, expect, eval = helpers.execute, helpers.expect, helpers.eval

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

local function enable_provider()
  execute('let g:clip_id = rpcstart("python2", ["test/functional/clipboard/clipboard_provider.py"])')
  -- wait for provider
  val = eval('rpcrequest(g:clip_id, "clipboard_get", "+")[1]')
  assert.are.equal(val, 'v')
end

describe('clipboard usage', function()
  setup(clear)
  it("works", function()
    basic_register_test()

    enable_provider()
    basic_register_test()

    -- "* and unnamed should function as independent registes
    insert("some words")
    feed('^"*dwdw"*P')
    expect('some ')
    clear()

    -- "* and "+ should be independent when the provider supports it
    insert([[
      text:
      first line
      secound line
      third line]])

    feed('G"+dd"*dddd"+p"*pp')
    expect([[
      text:
      third line
      secound line
      first line]])
    clear()

    execute('set clipboard=unnamed')

    -- the basic behavior of unnamed register should be the same
    -- even when handled by clipboard provider
    basic_register_test()

    -- with cb=unnamed, "* and unnamed will be the same register
    insert("some words")
    feed('^"*dwdw"*P')
    expect('words')
    clear()

  end)
end)
