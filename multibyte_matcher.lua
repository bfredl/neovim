function mismatch(char)
  if type(char) == "string" then
    char = vim.fn.char2nr(char)
  end
  local p = vim.api.nvim__inspect_char(char)
  local mis = {}
  if p.proc_ambiwidth and not p.ambiguous then
    mis.proc_ambiwidth = true
  elseif p.ambiguous and not p.proc_ambiwidth then
    mis.ambiguous = true
  end

  if p.proc_width == 2 and not p.doublewidth then
    mis.proc_width = true
  elseif p.doublewidth and not p.proc_width == 2 then
    mis.doublewidth = true
  end

  if p.proc_emoji_all and not p.emoji_all then
    mis.proc_emoji_all = true
  elseif p.emoji_all and not p.proc_emoji_all then
    mis.emoji_all = true
  end

  if p.proc_emoji_wide and not p.emoji_wide then
    mis.proc_emoji_wide = true
  elseif p.emoji_wide and not p.proc_emoji_wide then
    mis.emoji_wide = true
  end

  if p.proc_combining and not p.combining then
    mis.proc_combining = true
  elseif p.combining and not p.proc_combining then
    mis.combining = true
  end
  return mis
end

-- don't care about ASCII
sussy = {}
baka = {}
for i = 128,tonumber("0x1FFFF") do
  local m = mismatch(i)
  if next(m) ~= nil then
    table.insert(sussy, i)
  end
  for k, _ in pairs(m) do
    baka[k] = baka[k] or {}
    table.insert(baka[k], i)
  end
end

item, item2 = {}, {}
vim.tbl_keys(baka)
for _,val in pairs(baka.proc_emoji_wide) do
  table.insert(item, vim.fn.nr2char(val))
end
if baka.emoji_wide then
  for _,val in pairs(baka.emoji_wide) do
    table.insert(item2, vim.fn.nr2char(val))
  end
end
if false then
for _,val in pairs(baka.proc_emoji_wide) do
  print(vim.fn.printf("0x%x", val))
end
end

table.concat(item, " ")
table.concat(item2, " ")


vim.api.nvim__inspect_char(35)
vim.api.nvim__inspect_char(vim.fn.char2nr('本'))
vim.api.nvim__inspect_char(baka.proc_emoji_all[1])
vim.api.nvim__inspect_char(tonumber("0x1f1e6"))
vim.api.nvim__inspect_char(tonumber("0x1fa6d"))
vim.api.nvim__inspect_char(tonumber("0x1f321"))
vim.api.nvim__inspect_char(tonumber("0x1f320"))
vim.api.nvim__inspect_char(127995) -- E_modifier

vim.api.nvim__inspect_char(baka.proc_emoji_wide[1])
vim.api.nvim__inspect_char(baka.proc_emoji_wide[1]+4)

-- TODO: distinguish these two with proc_ data only?
-- NB: emoji_wide starts at 0x1f000, adding more emoji_all is fine..
vim.api.nvim__inspect_char(tonumber("0x1f321")) -- emoji
vim.api.nvim__inspect_char(tonumber("0x1f000")) -- extended pictogram but NOT emojid d
