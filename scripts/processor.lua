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
    end
    current = locations[locname]
    print(locname)
  elseif item[1] == "shadow" or item[1] == "anyfail" then
    if current then
      table.insert(current, item)
    else
      table.insert(stray_kids, item)
    end
    print("LOCATION", item[2], item[3])
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
  print(l)
  valid = true
  for _, item in ipairs(loc) do
    print(item[1])
    if item[1] == "anyfail" then
      valid = false
    elseif item[1] == "shadow" then
      locname = item[2]..':'..item[3]
      if newloc[locname] then
        valid = false
        table.insert(DUBLETTER, item)
      else
        newloc[locname] = true
      end
    end
  end

  if valid then
    table.insert(verified, loc)
  else
    table.insert(sussy, loc)
  end
end

