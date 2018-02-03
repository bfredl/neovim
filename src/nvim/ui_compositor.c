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
#include "nvim/ui_compositor.h"
#include "nvim/ugrid.h"
#include "nvim/api/private/helpers.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "ui_compositor.c.generated.h"
#endif

#define UI(b) (((UICompositorData *)b)->ui)


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "ui_events_compositor.generated.h"
#endif

int composed_uis = 0;

static UI *compositor = NULL;
static Integer curgrid;

void ui_compositor_init(void) {
  if (compositor != NULL) {
    return;
  }
  compositor = xcalloc(1, sizeof(UI));

  compositor->rgb = true;
  //compositor->stop = ui_compositor_stop;
  compositor->resize = ui_compositor_resize;
  //compositor->clear = ui_compositor_clear;
  //compositor->eol_clear = ui_compositor_eol_clear;
  //compositor->set_scroll_region = ui_compositor_set_scroll_region;
  //compositor->scroll = ui_compositor_scroll;
  //compositor->highlight_set = ui_compositor_highlight_set;
  //compositor->put = ui_compositor_put;
  //compositor->update_fg = ui_compositor_update_fg;
  //compositor->update_bg = ui_compositor_update_bg;
  //compositor->update_sp = ui_compositor_update_sp;
  //compositor->flush = ui_compositor_flush;
  //compositor->option_set = ui_compositor_option_set;
  compositor->grid_cursor_goto = ui_compositor_grid_cursor_goto;
  //compositor.float_info = ui_compositor_float_info;
  //compositor.float_close = ui_compositor_float_close;

  // Be unoptionated: will be attached togheter with a "real" ui anyway
  compositor->width = INT_MAX;
  compositor->height = INT_MAX;
  for (UIWidget i = 0; (int)i < UI_WIDGETS; i++) {
    compositor->ui_ext[i] = true;
  }

  curgrid = 1;
  ui_attach_impl(compositor);
}


void ui_compositor_attach(UI *ui)
{
  // ui_compositor_init()
  composed_uis++;
  ui->composed = true;
}

void ui_compositor_detach(UI *ui)
{
  composed_uis--;
  ui->composed = false;
}

bool compositor_active(void) {
  return composed_uis != 0;
}

static void ui_compositor_grid_cursor_goto(UI *ui, Integer grid, Integer x, Integer y)
{
  curgrid = grid;
  ui_compositor_call_cursor_goto(x,y);
}

static void ui_compositor_resize(UI *ui, Integer rows, Integer columns)
{
  if (curgrid == 1) {
    ui_compositor_call_resize(rows, columns);
  }
}

