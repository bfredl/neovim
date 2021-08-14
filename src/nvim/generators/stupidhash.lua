
if false then for _,f in pairs(vim.fn.api_info().functions) do
  print('  "'..f.name..'";')
end end

instrings = {
  "nvim_buf_line_count";
  "nvim_buf_attach";
  "nvim_buf_detach";
  "nvim_buf_get_lines";
  "nvim_buf_set_lines";
  "nvim_buf_set_text";
  "nvim_buf_get_offset";
  "nvim_buf_get_var";
  "nvim_buf_get_changedtick";
  "nvim_buf_get_keymap";
  "nvim_buf_set_keymap";
  "nvim_buf_del_keymap";
  "nvim_buf_get_commands";
  "nvim_buf_set_var";
  "nvim_buf_del_var";
  "nvim_buf_get_option";
  "nvim_buf_set_option";
  "nvim_buf_get_name";
  "nvim_buf_set_name";
  "nvim_buf_is_loaded";
  "nvim_buf_delete";
  "nvim_buf_is_valid";
  "nvim_buf_get_mark";
  "nvim_buf_get_extmark_by_id";
  "nvim_buf_get_extmarks";
  "nvim_buf_set_extmark";
  "nvim_buf_del_extmark";
  "nvim_buf_add_highlight";
  "nvim_buf_clear_namespace";
  "nvim_buf_call";
  "nvim_command_output";
  "nvim_execute_lua";
  "nvim_buf_get_number";
  "nvim_buf_clear_highlight";
  "nvim_buf_set_virtual_text";
  "buffer_insert";
  "buffer_get_line";
  "buffer_set_line";
  "buffer_del_line";
  "buffer_get_line_slice";
  "buffer_set_line_slice";
  "buffer_set_var";
  "buffer_del_var";
  "window_set_var";
  "window_del_var";
  "tabpage_set_var";
  "tabpage_del_var";
  "vim_set_var";
  "vim_del_var";
  "nvim_tabpage_list_wins";
  "nvim_tabpage_get_var";
  "nvim_tabpage_set_var";
  "nvim_tabpage_del_var";
  "nvim_tabpage_get_win";
  "nvim_tabpage_get_number";
  "nvim_tabpage_is_valid";
  "nvim_ui_attach";
  "ui_attach";
  "nvim_ui_detach";
  "nvim_ui_try_resize";
  "nvim_ui_set_option";
  "nvim_ui_try_resize_grid";
  "nvim_ui_pum_set_height";
  "nvim_ui_pum_set_bounds";
  "nvim_exec";
  "nvim_command";
  "nvim_get_hl_by_name";
  "nvim_get_hl_by_id";
  "nvim_get_hl_id_by_name";
  "nvim_set_hl";
  "nvim_feedkeys";
  "nvim_input";
  "nvim_input_mouse";
  "nvim_replace_termcodes";
  "nvim_eval";
  "nvim_exec_lua";
  "nvim_notify";
  "nvim_call_function";
  "nvim_call_dict_function";
  "nvim_strwidth";
  "nvim_list_runtime_paths";
  "nvim_get_runtime_file";
  "nvim_set_current_dir";
  "nvim_get_current_line";
  "nvim_set_current_line";
  "nvim_del_current_line";
  "nvim_get_var";
  "nvim_set_var";
  "nvim_del_var";
  "nvim_get_vvar";
  "nvim_set_vvar";
  "nvim_get_option";
  "nvim_get_all_options_info";
  "nvim_get_option_info";
  "nvim_set_option";
  "nvim_echo";
  "nvim_out_write";
  "nvim_err_write";
  "nvim_err_writeln";
  "nvim_list_bufs";
  "nvim_get_current_buf";
  "nvim_set_current_buf";
  "nvim_list_wins";
  "nvim_get_current_win";
  "nvim_set_current_win";
  "nvim_create_buf";
  "nvim_open_term";
  "nvim_chan_send";
  "nvim_open_win";
  "nvim_list_tabpages";
  "nvim_get_current_tabpage";
  "nvim_set_current_tabpage";
  "nvim_create_namespace";
  "nvim_get_namespaces";
  "nvim_paste";
  "nvim_put";
  "nvim_subscribe";
  "nvim_unsubscribe";
  "nvim_get_color_by_name";
  "nvim_get_color_map";
  "nvim_get_context";
  "nvim_load_context";
  "nvim_get_mode";
  "nvim_get_keymap";
  "nvim_set_keymap";
  "nvim_del_keymap";
  "nvim_get_commands";
  "nvim_get_api_info";
  "nvim_set_client_info";
  "nvim_get_chan_info";
  "nvim_list_chans";
  "nvim_call_atomic";
  "nvim_parse_expression";
  "nvim_list_uis";
  "nvim_get_proc_children";
  "nvim_get_proc";
  "nvim_select_popupmenu_item";
  "nvim_set_decoration_provider";
  "nvim_win_get_buf";
  "nvim_win_set_buf";
  "nvim_win_get_cursor";
  "nvim_win_set_cursor";
  "nvim_win_get_height";
  "nvim_win_set_height";
  "nvim_win_get_width";
  "nvim_win_set_width";
  "nvim_win_get_var";
  "nvim_win_set_var";
  "nvim_win_del_var";
  "nvim_win_get_option";
  "nvim_win_set_option";
  "nvim_win_get_position";
  "nvim_win_get_tabpage";
  "nvim_win_get_number";
  "nvim_win_is_valid";
  "nvim_win_set_config";
  "nvim_win_get_config";
  "nvim_win_hide";
  "nvim_win_close";
  "nvim_win_call";
  "buffer_line_count";
  "buffer_get_lines";
  "buffer_set_lines";
  "buffer_get_var";
  "buffer_get_option";
  "buffer_set_option";
  "buffer_get_name";
  "buffer_set_name";
  "buffer_is_valid";
  "buffer_get_mark";
  "buffer_add_highlight";
  "vim_command_output";
  "buffer_get_number";
  "buffer_clear_highlight";
  "tabpage_get_windows";
  "tabpage_get_var";
  "tabpage_get_window";
  "tabpage_is_valid";
  "ui_detach";
  "ui_try_resize";
  "vim_command";
  "vim_feedkeys";
  "vim_input";
  "vim_replace_termcodes";
  "vim_eval";
  "vim_call_function";
  "vim_strwidth";
  "vim_list_runtime_paths";
  "vim_change_directory";
  "vim_get_current_line";
  "vim_set_current_line";
  "vim_del_current_line";
  "vim_get_var";
  "vim_get_vvar";
  "vim_get_option";
  "vim_set_option";
  "vim_out_write";
  "vim_err_write";
  "vim_report_error";
  "vim_get_buffers";
  "vim_get_current_buffer";
  "vim_set_current_buffer";
  "vim_get_windows";
  "vim_get_current_window";
  "vim_set_current_window";
  "vim_get_tabpages";
  "vim_get_current_tabpage";
  "vim_set_current_tabpage";
  "vim_subscribe";
  "vim_unsubscribe";
  "vim_name_to_color";
  "vim_get_color_map";
  "vim_get_api_info";
  "window_get_buffer";
  "window_get_cursor";
  "window_set_cursor";
  "window_get_height";
  "window_set_height";
  "window_get_width";
  "window_set_width";
  "window_get_var";
  "window_get_option";
  "window_set_option";
  "window_get_position";
  "window_get_tabpage";
  "window_is_valid";
}

instrings = {
"id";
"end_line";
"end_col";
"hl_group";
"virt_text";
"virt_text_pos";
"virt_text_win_col";
"virt_text_hide";
"hl_eol";
"hl_mode";
"ephemeral";
"priority";
"right_gravity";
"end_right_gravity";
}

function setdefault(table, key)
  local val = table[key]
  if val == nil then
    val = {}
    table[key] = val
  end
  return val
end

function buckify(strings)
  local lenbucks = {}
  local maxlen = 0
  for _,s in ipairs(strings) do
    table.insert(setdefault(lenbucks, #s),s)
    if #s > maxlen then maxlen = #s end
  end

  local lenposbucks = {}
  local maxbucksize = 0

  for len = 1,maxlen do
    local strs = lenbucks[len]
    if strs then
      --print("len = "..len..", strs="..#strs)
      local minpos, minsize, buck = nil, #strs*2, nil
      for pos = 1,len do
        local posbucks = {}
        for _,str in ipairs(strs) do
          thechar = string.sub(str, pos, pos)
          --print(str, thechar)
          table.insert(setdefault(posbucks, thechar), str)
        end
        local maxsize = 1
        for c,strs2 in pairs(posbucks) do
          maxsize = math.max(maxsize, #strs2)
        end
        --print("pos = "..pos..", maxsize="..maxsize)
        if maxsize < minsize then
          minpos = pos
          minsize = maxsize
          buck = posbucks
        end
      end
      lenposbucks[len] = {minpos, buck}
      maxbucksize = math.max(maxbucksize, minsize)
    end
  end
  return lenposbucks, maxlen, maxbucksize
end

function switcher(tab, maxlen, maxbucksize)
  local neworder = {}
  local stats = {}
  local put = function(str) table.insert(stats, str) end
  put "switch (len) {\n"
  local bucky = maxbucksize > 1
  for len = 1,maxlen do
    local vals = tab[len]
    if vals then
      put("  case "..len..": ")
      local pos, posbuck = unpack(vals)
      local keys = vim.tbl_keys(posbuck)
      if #keys > 1 then
        table.sort(keys)
        put("switch (str["..(pos-1).."]) {\n")
        for _,c in ipairs(keys) do
          local buck = posbuck[c]
          local startidx = #neworder
          vim.list_extend(neworder, buck)
          local endidx = #neworder
          put("    case '"..c.."': ")
          put("low = "..startidx.."; ")
          if bucky then put("high = "..endidx.."; ") end
          put "break;\n"
        end
        put "    default: break;\n"
        put "  }\n  "
      else
          local startidx = #neworder
          table.insert(neworder, posbuck[keys[1]][1])
          local endidx = #neworder
          put("low = "..startidx.."; ")
          if bucky then put("high = "..endidx.."; ") end
      end
      put "break;\n"
    end
  end
  put "  default: break;\n"
  put "}"
  return neworder, table.concat(stats)
end
a,b,c = buckify(instrings)
x,y = switcher(a, b, c)
require 'luadev'.print(y)

function fakelookup(tab, strings) 
  local count, cmps = 0, 0
  for _, str in pairs(strings) do
    local pos, posbuck = unpack(tab[#str])
    local thechar = string.sub(str, pos, pos)
    local buck = posbuck[thechar]
    cmps = cmps + 1 + (#buck-1)/2.0
    count = count + 1
  end
  return count, cmps
end

--c
print(fakelookup(a, instrings))


--387/226

if false then

x = defaultdict()
x.y.z = 3

buckets = defaultdict()
for _,s in ipairs(instrings) do
  table.insert(buckets[#s],s)
end

for len,strs in pairs(buckets) do
  print("len = "..len..", strs="..#strs)
  minpos, minlen = nil, #strs+1
  for pos = 1,len do
    subbuck = defaultdict()
    for _,str in ipairs(strs) do
      thechar = string.sub(str, pos, pos)
      --print(str, thechar)
      table.insert(subbuck[thechar], str)
    end
    maxlen = 1
    for c,strs2 in pairs(subbuck) do
      maxlen = math.max(maxlen, #strs2)
    end
    --print("pos = "..pos..", maxlen="..maxlen)
    if maxlen < minlen then
      minpos = pos
      minlen = maxlen
    end
  end
  unilen = minlen
  for p1 = 1,len-1 do for p2 = p1+1,len do
    subbuck = defaultdict()
    for _,str in ipairs(strs) do
      c1 = string.sub(str, p1, p1)
      c2 = string.sub(str, p2, p2)
      --print(str, thechar)
      table.insert(subbuck[c1..c2], str)
    end
    maxlen = 1
    for c,strs2 in pairs(subbuck) do
      maxlen = math.max(maxlen, #strs2)
    end
    --print("pos = "..pos..", maxlen="..maxlen)
    if maxlen < minlen and (maxlen == 1 or maxlen <= unilen-2) then
      minpos = {p1,p2}
      minlen = maxlen
    end
  end end
  print("pos = "..vim.inspect(minpos)..", maxlen="..minlen)
  print()
end
end
