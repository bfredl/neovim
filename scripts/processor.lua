data = vim.json.decode(io.open('/tmp/case_file.json', 'rb'):read'*a')

current = nil
valid = true
locations = {}
loc_order = {}
stray_kids = {}

for _,item in ipairs(data) do
  if item[1] == "RESET" then
    current = nil
  elseif item[1] == "attr" then
    locname = item[2]..':'..item[3]
    if locations[locname] == nil then
      table.insert(loc_order, locname)
      locations[locname] = {}
      table.insert(locations[locname], item)
    end
    current = locations[locname]
    -- print(locname)
  elseif item[1] == "shadow" or item[1] == "anyfail" then
    if current then
      table.insert(current, item)
    else
      table.insert(stray_kids, item)
    end
    -- print("LOCATION", item[2], item[3])
  else
    error('BOOO')
  end
end

newloc = {}
DUBLETTER = {}

verified = {}
sussy = {}

for _,l in ipairs(loc_order) do
  loc = locations[l]
  -- print(l)
  valid = true

  for _, item in ipairs(loc) do
    -- print(item[1])
    if item[1] == "anyfail" then
      valid = false
    elseif item[1] == "attr" then
      buf = vim.fn.bufadd(item[2])
      vim.fn.bufload(buf)
      print(item[2], item[3])
      theline = vim.api.nvim_buf_get_lines(buf, item[3]-1,item[3], true)[1]
      item.linematch = vim.fn.match(theline, ":set_default_attr_ids")
      item.buf = buf
    elseif item[1] == "shadow" then
      locname = item[2]..':'..item[3]
      if newloc[locname] then
        valid = false
        table.insert(DUBLETTER, item)
      else
        newloc[locname] = true
      end
      texten = vim.split(item[4], "\n")
      item.texten = texten

      buf = vim.fn.bufadd(item[2])
      vim.fn.bufload(buf)
      item.theline = vim.api.nvim_buf_get_lines(buf, item[3]-1,item[3], true)[1]
      item.buf = buf
      -- TODO: [=[ and ]=]
      matches = vim.fn.matchbufline(buf, "[[", item[3], item[3]+3)
      if #matches > 0 then
        pos = matches[1]
        lnum_start = pos.lnum+1
        endline = lnum_start + #texten
        item.enline = vim.api.nvim_buf_get_lines(buf, endline-1,endline, true)[1]
        if vim.fn.match(item.enline, "]]") >= 0 then
          item.range = {lnum_start, endline-1}
        end
      end
    end
  end

  if valid then
    table.insert(verified, loc)
  else
    table.insert(sussy, loc)
  end
end

function begehen(items)
  for _, item in ipairs(loc) do
    if item[1] == "attr" then
      if item.linematch >= 0 then
        line = vim.api.nvim_buf_get_lines(buf, item[3]-1,item[3], true)[1]
        line = vim.fn.substitute(line, ":set_default_attr_ids", ":no_set_default_attr_ids", "")
        vim.api.nvim_buf_set_lines(buf, item[3]-1,item[3], true, {line})
      end
      item.linematch = vim.fn.match(theline, ":set_default_attr_ids")
      item.buf = buf
    elseif item[1] == "shadow" then
      if item.range ~= nil then
        a, b = unpack(item.range)
        vim.api.nvim_buf_set_lines(buf, a-1,b, true, item.texten)
      end
    end
  end
end

require'luadev'.print("OK")
for _,v in ipairs(verified) do
  require'luadev'.print(v[1][2], v[1][3])
end
require'luadev'.print("FAIL")
for _,v in ipairs(sussy) do
  require'luadev'.print(v[1][2], v[1][3])
end




--[[
verified[7]
--
for _,item in ipairs(verified) do begehen(item) end
begehen(verified[7])
verified[1]
#verified

  vim.split(verified[1][1][4], "\n")
--]]
