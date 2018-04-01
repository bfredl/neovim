// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// UI wrapper that sends requests to the UI thread.
// Used by the built-in TUI and libnvim-based UIs.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include "nvim/log.h"
#include "nvim/main.h"
#include "nvim/vim.h"
#include "nvim/ui.h"
#include "nvim/memory.h"
#include "nvim/ui_bridge.h"
#include "nvim/ugrid.h"
#include "nvim/api/private/helpers.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "ui_bridge.c.generated.h"
#endif

#define UI(b) (((UIBridgeData *)b)->ui)

// Schedule a function call on the UI bridge thread.
#define UI_BRIDGE_CALL(ui, name, argc, ...) \
  ((UIBridgeData *)ui)->scheduler( \
      event_create(ui_bridge_##name##_event, argc, __VA_ARGS__), UI(ui))

#define INT2PTR(i) ((void *)(intptr_t)i)
#define PTR2INT(p) ((Integer)(intptr_t)p)

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "ui_events_bridge.generated.h"
#endif

UI *ui_bridge_attach(UI *ui, ui_main_fn ui_main, event_scheduler scheduler)
{
  UIBridgeData *rv = xcalloc(1, sizeof(UIBridgeData));
  rv->ui = ui;
  rv->bridge.rgb = ui->rgb;
  rv->bridge.stop = ui_bridge_stop;
  rv->bridge.resize = ui_bridge_resize;
  rv->bridge.clear = ui_bridge_clear;
  rv->bridge.eol_clear = ui_bridge_eol_clear;
  rv->bridge.cursor_goto = ui_bridge_cursor_goto;
  rv->bridge.mode_info_set = ui_bridge_mode_info_set;
  rv->bridge.update_menu = ui_bridge_update_menu;
  rv->bridge.busy_start = ui_bridge_busy_start;
  rv->bridge.busy_stop = ui_bridge_busy_stop;
  rv->bridge.mouse_on = ui_bridge_mouse_on;
  rv->bridge.mouse_off = ui_bridge_mouse_off;
  rv->bridge.mode_change = ui_bridge_mode_change;
  rv->bridge.set_scroll_region = ui_bridge_set_scroll_region;
  rv->bridge.scroll = ui_bridge_scroll;
  rv->bridge.hl_attr_define = ui_bridge_hl_attr_define;
  rv->bridge.hl_attr_set = ui_bridge_hl_attr_set;
  rv->bridge.put = ui_bridge_put;
  rv->bridge.bell = ui_bridge_bell;
  rv->bridge.visual_bell = ui_bridge_visual_bell;
  rv->bridge.default_colors_set = ui_bridge_default_colors_set;
  rv->bridge.flush = ui_bridge_flush;
  rv->bridge.suspend = ui_bridge_suspend;
  rv->bridge.set_title = ui_bridge_set_title;
  rv->bridge.set_icon = ui_bridge_set_icon;
  rv->bridge.option_set = ui_bridge_option_set;
  rv->bridge.raw_line = ui_bridge_raw_line;
  rv->scheduler = scheduler;

  for (UIExtension i = 0; (int)i < kUIExtCount; i++) {
    rv->bridge.ui_ext[i] = ui->ui_ext[i];
  }

  rv->ui_main = ui_main;
  uv_mutex_init(&rv->mutex);
  uv_cond_init(&rv->cond);
  uv_mutex_lock(&rv->mutex);
  rv->ready = false;

  if (uv_thread_create(&rv->ui_thread, ui_thread_run, rv)) {
    abort();
  }

  // Suspend the main thread until CONTINUE is called by the UI thread.
  while (!rv->ready) {
    uv_cond_wait(&rv->cond, &rv->mutex);
  }
  uv_mutex_unlock(&rv->mutex);

  ui_attach_impl(&rv->bridge);
  return &rv->bridge;
}

void ui_bridge_stopped(UIBridgeData *bridge)
{
  uv_mutex_lock(&bridge->mutex);
  bridge->stopped = true;
  uv_mutex_unlock(&bridge->mutex);
}

static void ui_thread_run(void *data)
{
  UIBridgeData *bridge = data;
  bridge->ui_main(bridge, bridge->ui);
}

static void ui_bridge_stop(UI *b)
{
  // Detach brigde first, so that "stop" is the last event the TUI loop
  // receives from the main thread. #8041
  ui_detach_impl(b);
  UIBridgeData *bridge = (UIBridgeData *)b;
  bool stopped = bridge->stopped = false;
  UI_BRIDGE_CALL(b, stop, 1, b);
  for (;;) {
    uv_mutex_lock(&bridge->mutex);
    stopped = bridge->stopped;
    uv_mutex_unlock(&bridge->mutex);
    if (stopped) {
      break;
    }
    loop_poll_events(&main_loop, 10);  // Process one event.
  }
  uv_thread_join(&bridge->ui_thread);
  uv_mutex_destroy(&bridge->mutex);
  uv_cond_destroy(&bridge->cond);
  xfree(bridge->ui);  // Threads joined, now safe to free UI container. #7922
  xfree(b);
}
static void ui_bridge_stop_event(void **argv)
{
  UI *ui = UI(argv[0]);
  ui->stop(ui);
}

static void ui_bridge_hl_attr_define(UI *ui, Integer id, HlAttrs attrs, Dictionary info)
{
  HlAttrs *a = xmalloc(sizeof(HlAttrs));
  *a = attrs;
  UI_BRIDGE_CALL(ui, hl_attr_define, 3, ui, INT2PTR(id), a);
}
static void ui_bridge_hl_attr_define_event(void **argv)
{
  UI *ui = UI(argv[0]);
  Dictionary info = ARRAY_DICT_INIT;
  ui->hl_attr_define(ui, PTR2INT(argv[1]), *((HlAttrs *)argv[2]), info);
  xfree(argv[2]);
}

 
static void ui_bridge_raw_line_event(void **argv)
{
  UI *ui = UI(argv[0]);
  ui->raw_line(ui, PTR2INT(argv[1]), PTR2INT(argv[2]), PTR2INT(argv[3]), PTR2INT(argv[4]), argv[5], argv[6]);
  xfree(argv[5]);
  xfree(argv[6]);
}

static void ui_bridge_raw_line(UI *ui, Integer row, Integer startcol, Integer endcol, Integer clearcol, schar_T *chunk, sattr_T *attrs)
{
  schar_T *c = xmemdup(chunk,(endcol-startcol)*sizeof(schar_T));
  sattr_T *hl = xmemdup(attrs,(endcol-startcol)*sizeof(sattr_T));
  UI_BRIDGE_CALL(ui, raw_line, 7, ui, INT2PTR(row), INT2PTR(startcol), INT2PTR(endcol), INT2PTR(clearcol), c, hl);
}


static void ui_bridge_suspend(UI *b)
{
  UIBridgeData *data = (UIBridgeData *)b;
  uv_mutex_lock(&data->mutex);
  UI_BRIDGE_CALL(b, suspend, 1, b);
  data->ready = false;
  // Suspend the main thread until CONTINUE is called by the UI thread.
  while (!data->ready) {
    uv_cond_wait(&data->cond, &data->mutex);
  }
  uv_mutex_unlock(&data->mutex);
}
static void ui_bridge_suspend_event(void **argv)
{
  UI *ui = UI(argv[0]);
  ui->suspend(ui);
}
