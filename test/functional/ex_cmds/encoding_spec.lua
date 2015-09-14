local helpers = require('test.functional.helpers')
local clear, execute, feed = helpers.clear, helpers.execute, helpers.feed
local eq, neq, eval = helpers.eq, helpers.neq, helpers.eval

describe('&encoding', function()

  before_each(function()
    clear()
    -- sanity check: tests should run with encoding=utf-8
    eq('utf-8', eval('&encoding'))
    eq(3, eval('strwidth("Bär")'))
  end)

  it('cannot be changed after setup', function()
    execute('set encoding=latin1')
    -- error message expected
    feed('<cr>')
    eq('utf-8', eval('&encoding'))
    -- check nvim is still in utf-8 mode
    eq(3, eval('strwidth("Bär")'))
  end)

  it('cannot be changed during startup', function()
    clear('set enc=latin1')
    feed('<cr>')
    eq('utf-8', eval('&encoding'))
    eq(3, eval('strwidth("Bär")'))
  end)

end)
