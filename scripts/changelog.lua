-- Executes and returns the output of `cmd`, or nil on failure.
--
-- Prints `cmd` if `trace` is enabled.
local function run(cmd, or_die)
  if _trace then
    p('run: '..vim.inspect(cmd))
  end
  local rv = vim.trim(vim.fn.system(cmd)) or ''
  if vim.v.shell_error ~= 0 then
    if or_die then
      p(rv)
      die()
    end
    return nil
  end
  return rv
end
_G.run = run

local lastver = 'v0.6.0'

-- local
outp = run({'git', 'rev-list', lastver..'..HEAD'}, true)
commits = vim.split(outp, '\n')
--print(#commits)


messages = {}
for _, c in ipairs(commits) do
  local msg = run({'git', 'show', '-s', '--format=%B' , c})
  table.insert(messages, msg)
end

buf = vim.api.nvim_create_buf(true, false)
for _, m in ipairs(messages) do
  vim.api.nvim_buf_set_lines(buf, -1, -1, true, vim.split(m, '[\n\r]'))
end

vim.api.nvim_buf_call(buf, function()
  vim.cmd [[
    set ignorecase
    v/^\a\+\((.*)\|\)!\?:/d
    g/\(vim-patch\|problem\|solution\|https\):/d
    sort u
  ]]
end)

print("THEBUF", buf)
buf


