// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nvim/vim.h"
#include "nvim/ui.h"
#include "nvim/memory.h"
#include "nvim/map.h"
#include "nvim/msgpack_rpc/channel.h"
#include "nvim/api/ui.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/helpers.h"
#include "nvim/popupmnu.h"
#include "nvim/cursor_shape.h"
#include "nvim/highlight.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "api/ui.c.generated.h"
# include "ui_events_remote.generated.h"
#endif

typedef struct {
  uint64_t channel_id;
  Array buffer;
  int hl_id;
  Integer cursor_row, cursor_col;
  Integer client_row, client_col;
} UIData;

static PMap(uint64_t) *connected_uis = NULL;

void remote_ui_init(void)
  FUNC_API_NOEXPORT
{
  connected_uis = pmap_new(uint64_t)();
}

void remote_ui_disconnect(uint64_t channel_id)
  FUNC_API_NOEXPORT
{
  UI *ui = pmap_get(uint64_t)(connected_uis, channel_id);
  if (!ui) {
    return;
  }
  UIData *data = ui->data;
  api_free_array(data->buffer);  // Destroy pending screen updates.
  pmap_del(uint64_t)(connected_uis, channel_id);
  xfree(ui->data);
  ui->data = NULL;  // Flag UI as "stopped".
  ui_detach_impl(ui);
  xfree(ui);
}

void nvim_ui_attach(uint64_t channel_id, Integer width, Integer height,
                    Dictionary options, Error *err)
  FUNC_API_SINCE(1) FUNC_API_REMOTE_ONLY
{
  if (pmap_has(uint64_t)(connected_uis, channel_id)) {
    api_set_error(err, kErrorTypeException,
                  "UI already attached to channel: %" PRId64, channel_id);
    return;
  }

  if (width <= 0 || height <= 0) {
    api_set_error(err, kErrorTypeValidation,
                  "Expected width > 0 and height > 0");
    return;
  }
  UI *ui = xcalloc(1, sizeof(UI));
  ui->width = (int)width;
  ui->height = (int)height;
  ui->rgb = true;
  ui->resize = remote_ui_resize;
  ui->clear = remote_ui_clear;
  ui->cursor_goto = remote_ui_grid_cursor_goto;
  ui->mode_info_set = remote_ui_mode_info_set;
  ui->update_menu = remote_ui_update_menu;
  ui->busy_start = remote_ui_busy_start;
  ui->busy_stop = remote_ui_busy_stop;
  ui->mouse_on = remote_ui_mouse_on;
  ui->mouse_off = remote_ui_mouse_off;
  ui->mode_change = remote_ui_mode_change;
  ui->grid_scroll = remote_ui_grid_scroll;
  ui->hl_attr_define = remote_ui_hl_attr_define;
  ui->raw_line = remote_ui_raw_line;
  ui->bell = remote_ui_bell;
  ui->visual_bell = remote_ui_visual_bell;
  ui->default_colors_set = remote_ui_default_colors_set;
  ui->flush = remote_ui_flush;
  ui->suspend = remote_ui_suspend;
  ui->set_title = remote_ui_set_title;
  ui->set_icon = remote_ui_set_icon;
  ui->option_set = remote_ui_option_set;
  ui->event = remote_ui_event;

  memset(ui->ui_ext, 0, sizeof(ui->ui_ext));

  for (size_t i = 0; i < options.size; i++) {
    ui_set_option(ui, options.items[i].key, options.items[i].value, err);
    if (ERROR_SET(err)) {
      xfree(ui);
      return;
    }
  }

  UIData *data = xmalloc(sizeof(UIData));
  data->channel_id = channel_id;
  data->buffer = (Array)ARRAY_DICT_INIT;
  data->hl_id = 0;
  ui->data = data;

  pmap_put(uint64_t)(connected_uis, channel_id, ui);
  ui_attach_impl(ui);
}

/// @deprecated
void ui_attach(uint64_t channel_id, Integer width, Integer height,
               Boolean enable_rgb, Error *err)
{
  Dictionary opts = ARRAY_DICT_INIT;
  PUT(opts, "rgb", BOOLEAN_OBJ(enable_rgb));
  nvim_ui_attach(channel_id, width, height, opts, err);
  api_free_dictionary(opts);
}

void nvim_ui_detach(uint64_t channel_id, Error *err)
  FUNC_API_SINCE(1) FUNC_API_REMOTE_ONLY
{
  if (!pmap_has(uint64_t)(connected_uis, channel_id)) {
    api_set_error(err, kErrorTypeException,
                  "UI not attached to channel: %" PRId64, channel_id);
    return;
  }
  remote_ui_disconnect(channel_id);
}


void nvim_ui_try_resize(uint64_t channel_id, Integer width,
                        Integer height, Error *err)
  FUNC_API_SINCE(1) FUNC_API_REMOTE_ONLY
{
  if (!pmap_has(uint64_t)(connected_uis, channel_id)) {
    api_set_error(err, kErrorTypeException,
                  "UI not attached to channel: %" PRId64, channel_id);
    return;
  }

  if (width <= 0 || height <= 0) {
    api_set_error(err, kErrorTypeValidation,
                  "Expected width > 0 and height > 0");
    return;
  }

  UI *ui = pmap_get(uint64_t)(connected_uis, channel_id);
  ui->width = (int)width;
  ui->height = (int)height;
  ui_refresh();
}

void nvim_ui_set_option(uint64_t channel_id, String name,
                        Object value, Error *error)
  FUNC_API_SINCE(1) FUNC_API_REMOTE_ONLY
{
  if (!pmap_has(uint64_t)(connected_uis, channel_id)) {
    api_set_error(error, kErrorTypeException,
                  "UI not attached to channel: %" PRId64, channel_id);
    return;
  }
  UI *ui = pmap_get(uint64_t)(connected_uis, channel_id);

  ui_set_option(ui, name, value, error);
  if (!ERROR_SET(error)) {
    ui_refresh();
  }
}

static void ui_set_option(UI *ui, String name, Object value, Error *error)
{
  if (strequal(name.data, "rgb")) {
    if (value.type != kObjectTypeBoolean) {
      api_set_error(error, kErrorTypeValidation, "rgb must be a Boolean");
      return;
    }
    ui->rgb = value.data.boolean;
    return;
  }

  for (UIExtension i = 0; i < kUIExtCount; i++) {
    if (strequal(name.data, ui_ext_names[i])) {
      if (value.type != kObjectTypeBoolean) {
        snprintf((char *)IObuff, IOSIZE, "%s must be a Boolean",
                 ui_ext_names[i]);
        api_set_error(error, kErrorTypeValidation, (char *)IObuff);
        return;
      }
      ui->ui_ext[i] = value.data.boolean;
      return;
    }
  }

  if (strequal(name.data, "popupmenu_external")) {
    // LEGACY: Deprecated option, use `ext_cmdline` instead.
    if (value.type != kObjectTypeBoolean) {
      api_set_error(error, kErrorTypeValidation,
                    "popupmenu_external must be a Boolean");
      return;
    }
    ui->ui_ext[kUIPopupmenu] = value.data.boolean;
    return;
  }

  api_set_error(error, kErrorTypeValidation, "No such UI option");
#undef UI_EXT_OPTION
}

/// Pushes data into UI.UIData, to be consumed later by remote_ui_flush().
static void push_call(UI *ui, char *name, Array args)
{
  Array call = ARRAY_DICT_INIT;
  UIData *data = ui->data;

  // To optimize data transfer(especially for "put"), we bundle adjacent
  // calls to same method together, so only add a new call entry if the last
  // method call is different from "name"
  if (kv_size(data->buffer)) {
    call = kv_A(data->buffer, kv_size(data->buffer) - 1).data.array;
  }

  if (!kv_size(call) || strcmp(kv_A(call, 0).data.string.data, name)) {
    call = (Array)ARRAY_DICT_INIT;
    ADD(data->buffer, ARRAY_OBJ(call));
    ADD(call, STRING_OBJ(cstr_to_string(name)));
  }

  ADD(call, ARRAY_OBJ(args));
  kv_A(data->buffer, kv_size(data->buffer) - 1).data.array = call;
}

static void remote_ui_grid_scroll(UI *ui, Integer g, Integer top, Integer bot,
                            Integer left, Integer right, Integer rows, Integer cols)
{

  Array args = ARRAY_DICT_INIT;
  ADD(args, INTEGER_OBJ(top));
  ADD(args, INTEGER_OBJ(bot-1));
  ADD(args, INTEGER_OBJ(left));
  ADD(args, INTEGER_OBJ(right-1));
  push_call(ui, "set_scroll_region", args);

  args = (Array)ARRAY_DICT_INIT;
  ADD(args, INTEGER_OBJ(rows));
  push_call(ui, "scroll", args);

}

static void remote_ui_default_colors_set(UI *ui, Integer rgb_fg, Integer rgb_bg, Integer rgb_sp, Integer cterm_fg, Integer cterm_bg)
{
  Array args = ARRAY_DICT_INIT;
  ADD(args, INTEGER_OBJ(rgb_fg));
  ADD(args, INTEGER_OBJ(rgb_bg));
  ADD(args, INTEGER_OBJ(rgb_sp));
  ADD(args, INTEGER_OBJ(cterm_fg));
  ADD(args, INTEGER_OBJ(cterm_bg));
  push_call(ui, "default_colors_set", args);

  // Deprecated
  if (!ui->ui_ext[kUIMultigrid]) {
    args = (Array)ARRAY_DICT_INIT;
    ADD(args, INTEGER_OBJ(ui->rgb ? normal_fg : cterm_normal_fg_color - 1));
    push_call(ui, "update_fg", args);

    args = (Array)ARRAY_DICT_INIT;
    ADD(args, INTEGER_OBJ(ui->rgb ? normal_bg : cterm_normal_bg_color - 1));
    push_call(ui, "update_bg", args);

    args = (Array)ARRAY_DICT_INIT;
    ADD(args, INTEGER_OBJ(ui->rgb ? normal_bg : cterm_normal_bg_color - 1));
    push_call(ui, "update_sp", args);
  }
}

static void remote_ui_hl_attr_define(UI *ui, Integer id, HlAttrs attrs, Array info)
{
  if (!ui->ui_ext[kUIMultigrid]) {
    return;
  }
  Array args = ARRAY_DICT_INIT;
  Dictionary hl = hlattrs2dict(&attrs, kNone);

  ADD(args, INTEGER_OBJ(id));
  ADD(args, DICTIONARY_OBJ(hl));
  if (ui->ui_ext[kUIHlState]) {
    ADD(args, ARRAY_OBJ(copy_array(info)));
  } else {
    // TODO: actually map the same hls to the same id also
    ADD(args, ARRAY_OBJ((Array)ARRAY_DICT_INIT));
  }

  push_call(ui, "hl_attr_define", args);
}

static void remote_ui_highlight_set(UI *ui, int id)
{
  Array args = ARRAY_DICT_INIT;
  UIData *data = ui->data;

  HlAttrs attrs = HLATTRS_INIT;

  if (data->hl_id == id) {
    return;
  }
  data->hl_id = id;

  if (id != 0) {
    HlAttrs *aep = syn_cterm_attr2entry(id);
    if (aep) {
      attrs = *aep;
    }
  }

  Dictionary hl = hlattrs2dict(&attrs, ui->rgb ? kTrue : kFalse);

  ADD(args, DICTIONARY_OBJ(hl));
  push_call(ui, "highlight_set", args);
}

static void remote_ui_grid_cursor_goto(UI *ui, Integer row, Integer col)
{
  UIData *data = ui->data;
  data->cursor_row = row;
  data->cursor_col = col;
  remote_ui_cursor_goto(ui, row, col);
}


static void remote_ui_cursor_goto(UI *ui, Integer row, Integer col)
{
  UIData *data = ui->data;
  if (data->client_row == row && data->client_col == col) {
    return;
  }
  data->client_row = row;
  data->client_col = col;
  Array args = ARRAY_DICT_INIT;
  ADD(args, INTEGER_OBJ(row));
  ADD(args, INTEGER_OBJ(col));
  push_call(ui, "cursor_goto", args);
}

static void remote_ui_put(UI *ui, const char *cell)
{
  UIData *data = ui->data;
  data->client_col++;
  Array args = ARRAY_DICT_INIT;
  ADD(args, STRING_OBJ(cstr_to_string(cell)));
  push_call(ui, "put", args);
}

static void remote_ui_raw_line(UI *ui, Integer row, Integer startcol, Integer endcol, Integer clearcol, Integer clearattr, schar_T *chunk, sattr_T *attrs)
{
  UIData *data = ui->data;
  if (ui->ui_ext[kUIMultigrid]) {
    Array args = ARRAY_DICT_INIT;
    ADD(args, INTEGER_OBJ(row));
    ADD(args, INTEGER_OBJ(startcol));
    // TODO: this is messy, use [utf8, attr, repeat] instead!
    ADD(args, INTEGER_OBJ(clearcol));
    ADD(args, INTEGER_OBJ(clearattr));
    Array cells = ARRAY_DICT_INIT;
    int last_hl = -1;
    for (int i = 0; i < endcol-startcol; i++) {
      Array cell = ARRAY_DICT_INIT;
      ADD(cell, STRING_OBJ(cstr_to_string((const char *)chunk[i])));
      if (attrs[i] != last_hl) {
        ADD(cell, INTEGER_OBJ(attrs[i]));
        last_hl = attrs[i];
      }
      ADD(cells, ARRAY_OBJ(cell));
    }
    ADD(args, ARRAY_OBJ(cells));

    push_call(ui, "line", args);
  } else {
    for (int i = 0; i < endcol-startcol; i++) {
      remote_ui_cursor_goto(ui, row, startcol+i);
      remote_ui_highlight_set(ui, attrs[i]);
      remote_ui_put(ui, (const char *)chunk[i]);
      if (utf_ambiguous_width(utf_ptr2char(chunk[i]))) {
        data->client_col = -1; // force cursor update
      }
    }
    if (endcol < clearcol) {
      remote_ui_highlight_set(ui, (int)clearattr);
      if (clearattr== 0 &&  clearcol == Columns) {
        remote_ui_cursor_goto(ui, row, endcol);
        Array args = ARRAY_DICT_INIT;
        push_call(ui, "eol_clear", args);
      } else {
        remote_ui_cursor_goto(ui, row, endcol);
        for (Integer c = endcol; c < clearcol; c++) {
          remote_ui_put(ui, " ");
        }
      }
    }
  }
}

static void remote_ui_flush(UI *ui)
{
  UIData *data = ui->data;
  if (!ui->ui_ext[kUIMultigrid]) {
    remote_ui_cursor_goto(ui, data->cursor_row, data->cursor_col);
  }
  if (data->buffer.size > 0) {
    rpc_send_event(data->channel_id, "redraw", data->buffer);
    data->buffer = (Array)ARRAY_DICT_INIT;
  }
}

static void remote_ui_cmdline_show(UI *ui, Array args)
{
  Array new_args = ARRAY_DICT_INIT;
  Array contents = args.items[0].data.array;
  Array new_contents = ARRAY_DICT_INIT;
  for (size_t i = 0; i < contents.size; i++) {
    Array item = contents.items[i].data.array;
    Array new_item = ARRAY_DICT_INIT;
    int attr = (int)item.items[0].data.integer;
    if (attr) {
      HlAttrs *aep = syn_cterm_attr2entry(attr);
      Dictionary rgb_attrs = hlattrs2dict(aep, ui->rgb ? kTrue : kFalse);
      ADD(new_item, DICTIONARY_OBJ(rgb_attrs));
    } else {
      ADD(new_item, DICTIONARY_OBJ((Dictionary)ARRAY_DICT_INIT));
    }
    ADD(new_item, copy_object(item.items[1]));
    ADD(new_contents, ARRAY_OBJ(new_item));
  }
  ADD(new_args, ARRAY_OBJ(new_contents));
  for (size_t i = 1; i < args.size; i++) {
    ADD(new_args, copy_object(args.items[i]));
  }
  push_call(ui, "cmdline_show", new_args);
}

static void remote_ui_event(UI *ui, char *name, Array args, bool *args_consumed)
{
  if (!ui->ui_ext[kUIMultigrid]) {
    // the representation of cmdline_show changed, translate back
    if (strequal(name, "cmdline_show")) {
      remote_ui_cmdline_show(ui, args);
      // never consumes args
      return;
    }
  }
  Array my_args = ARRAY_DICT_INIT;
  // Objects are currently single-reference
  // make a copy, but only if necessary
  if (*args_consumed) {
    for (size_t i = 0; i < args.size; i++) {
      ADD(my_args, copy_object(args.items[i]));
    }
  } else {
    my_args = args;
    *args_consumed = true;
  }
  push_call(ui, name, my_args);
}
