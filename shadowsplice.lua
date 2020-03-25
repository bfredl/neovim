local a = vim.api
_G.a = vim.api
local luadev = require'luadev'
local ssp = _G.ssp or {}
_G.ssp = ssp

ssp.ns = a.nvim_create_namespace("ssp")

function ssp.start(bufnr)
  if bufnr == nil or bufnr == 0 then
    bufnr = a.nvim_get_current_buf()
  end
  ssp.bufnr = bufnr
  ssp.reset()
  a.nvim_buf_attach(bufnr, false, {on_bytes=function(...)
    ssp.on_bytes(...)
  end})
end

function ssp.reset()
  local text = a.nvim_buf_get_lines(ssp.bufnr, 0, -1, true)
  local bytes = table.concat(text, '\n') .. '\n'
  ssp.shadow = bytes
  ssp.dirty = false
end

function ssp.on_bytes(_, buf, tick, start_row, start_col, start_byte, old_row, old_col, old_byte, new_row, new_col, new_byte)
  local before = string.sub(ssp.shadow, 1, start_byte)
  -- assume no text will contain 0xff bytes (invalid UTF-8)
  -- so we can use it as marker for unknown bytes
  local unknown = string.rep('\255', new_byte)
  local after = string.sub(ssp.shadow, start_byte + old_byte + 1)
  ssp.shadow = before .. unknown .. after
end

function ssp.sync()
  local text = meths.buf_get_lines(0, 0, -1, true)
  local bytes = table.concat(text, '\n') .. '\n'
  for i = 1, string.len(ssp.shadow) do
    local shadowbyte = string.sub(ssp.shadow, i, i)
    if shadowbyte ~= '\255' then
      if string.sub(bytes, i, i) ~= shadowbyte then
        error(i)
      end
    end
  end
end

function ssp.show()
  a.nvim_buf_clear_namespace(ssp.bufnr, ssp.ns, 0, -1)
  local text = meths.buf_get_lines(ssp.bufnr, 0, -1, true)
  local bytes = table.concat(text, '\n') .. '\n'
  local line, lastpos = 0, 0
  for i = 1, string.len(ssp.shadow) do
    if textbyte == '\n' then
      line = line + 1
      lastpos = i
    end
    local textbyte = string.sub(bytes, i, i)
    local shadowbyte = string.sub(ssp.shadow, i, i)
    if shadowbyte ~= '\255' then
      if textbyte ~= shadowbyte then
        a.nvim_buf_add_highlight(ssp.bufnr, ssp.ns, "ErrorMsg", line, i-lastpos, i-lastpos+1)
      end
    else
        a.nvim_buf_add_highlight(ssp.bufnr, ssp.ns, "MoreMsg", line, i-lastpos, i-lastpos+1)
    end
  end
end

local a = vim.api
function thecb(...)
  local yarg = {...}
  vim.schedule(function() luadev.print(vim.inspect(yarg)) end)
  -- args are "nvim_buf_lines_event", 1, 85, 8, 8, { "" }, false
end

function reg()
  a.nvim_buf_attach(0, false, {on_bytes=thecb})
end

reg()

ra = [[
bx
yb
  vim.schedule(function() luadev.print(vim.inspect(yarg)) end)
  -- args are "nvim_buf_lines_event", 1, 85, 8, 8, { "" }, false
bb
]]
