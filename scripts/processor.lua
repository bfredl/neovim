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
      -- vim.bo[buf].buflisted = true
      --print(item[2], item[3])
      theline = vim.api.nvim_buf_get_lines(buf, item[3]-1,item[3], true)[1]
      item.linematch = vim.fn.match(theline, ":set_default_attr_ids")
      item.prefix = vim.fn.matchlist(theline,"^\\s*")[1]
      item.endline = vim.api.nvim_buf_call(buf, function()
        vim.api.nvim_win_set_cursor(0, {item[3], 0})
        vim.cmd 'normal f{'
        return vim.fn.searchpair('{', '', '}')
      end)
      item.buf = buf
    elseif item[1] == "shadow" then
      locname = item[2]..':'..item[3]
      if newloc[locname] then
        --valid = false
        table.insert(DUBLETTER, item)
      else
        newloc[locname] = true
      end
      texten = vim.split(item[4], "\n")
      item.texten = texten

      buf = vim.fn.bufadd(item[2])
      vim.fn.bufload(buf)
      -- vim.bo[buf].buflisted = true
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
          item.fragile = vim.fn.matchbufline(buf, "\\CMATCH",  item[3]+1, endline-1)
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

interentions = nil
function begehen(items)
  for _, item in ipairs(items) do
    if item[1] == "attr" then
      if item.linematch >= 0 then
        line = vim.api.nvim_buf_get_lines(item.buf, item[3]-1,item[3], true)[1]
        line = vim.fn.substitute(line, ":set_default_attr_ids", ":no_set_default_attr_ids", "")
        vim.api.nvim_buf_set_lines(item.buf, item[3]-1,item[3], true, {line})
        interventions[item[2]] = interventions[item[2]] or {}
        interventions[item[2]][item[3]] = item
        print(item[4])
      end
    elseif item[1] == "shadow" then
      if item.range ~= nil then
        a, b = unpack(item.range)
        print("mod", item.buf, item[2])
        vim.api.nvim_buf_set_lines(item.buf, a-1,b, true, item.texten)
        if #item.fragile > 0 then
          print("!!!!!!FRAGIL")
        end
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


function intervene()
  for k,v in pairs(interventions) do
      print("fina filen: "..k)
      _G.v = v
      keys = vim.tbl_keys(v)
      table.sort(keys, function(x,y) return x > y end)
      for _,k in ipairs(keys) do
        item = v[k]
        if item.endline > item[3] then
          texten = item[4] and vim.split(item[4], "\n") or {}
          for i = 1,#texten do
            texten[i] = item.prefix..texten[i]
          end
          vim.api.nvim_buf_set_lines(item.buf, item[3]-1,item.endline, true, texten)
        end
      end
  end
end


--[[
verified[2]
verified[7]
--
interventions = {}
for _,item in ipairs(verified) do begehen(item) end
intervene()


begehen(verified[7])
verified[1]
#verified

  vim.split(verified[1][1][4], "\n")
--]]
