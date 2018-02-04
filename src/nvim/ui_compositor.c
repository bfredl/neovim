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

static ScreenGrid *grid, *float_grid;

static int row,col;
bool curinvalid;

void ui_compositor_init(void) {
  if (compositor != NULL) {
    return;
  }
  compositor = xcalloc(1, sizeof(UI));

  compositor->rgb = true;
  //compositor->stop = ui_compositor_stop;
  compositor->resize = ui_compositor_resize;
  //compositor->clear = ui_compositor_clear;
  compositor->eol_clear = ui_compositor_eol_clear;
  //compositor->set_scroll_region = ui_compositor_set_scroll_region;
  //compositor->scroll = ui_compositor_scroll;
  //compositor->highlight_set = ui_compositor_highlight_set;
  compositor->put = ui_compositor_put;
  //compositor->update_fg = ui_compositor_update_fg;
  //compositor->update_bg = ui_compositor_update_bg;
  //compositor->update_sp = ui_compositor_update_sp;
  //compositor->flush = ui_compositor_flush;
  //compositor->option_set = ui_compositor_option_set;
  compositor->grid_cursor_goto = ui_compositor_grid_cursor_goto;
  //compositor.float_info = ui_compositor_float_info;
  //compositor.float_close = ui_compositor_float_close;

  // Be unoptionated: will be attached together with a "real" ui anyway
  compositor->width = INT_MAX;
  compositor->height = INT_MAX;
  for (UIWidget i = 0; (int)i < UI_WIDGETS; i++) {
    compositor->ui_ext[i] = true;
  }

  curgrid = 1;
  grid = &default_grid;
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

void ui_compositor_set_grid(ScreenGrid *new_grid)
{
  if (new_grid->handle != 1) {
    float_grid = new_grid;
  }
  grid = new_grid;
}

void ui_compositor_remove_grid(ScreenGrid *g)
{
  assert(grid != g);
  if (float_grid == g) {
    float_grid = NULL;
  }
}

static void ui_compositor_grid_cursor_goto(UI *ui, Integer grid_handle, Integer r, Integer c)
{
  assert(grid->handle == grid_handle);
  row = r + grid->comp_y;
  col = c + grid->comp_x;
  // TODO: this should be assured downstream instead
  if (col < default_grid.Columns && row < default_grid.Rows) {
    ui_compositor_call_cursor_goto(row,col);
    curinvalid = false;
  }
}

static void ui_compositor_put(UI *ui, String str)
{
  if (grid != float_grid && float_grid != NULL
    && float_grid->comp_x <= col && col < float_grid->comp_x + float_grid->Columns
    && float_grid->comp_y <= row && row < float_grid->comp_y + float_grid->Rows) {
      curinvalid = true;
  } else {
    if (curinvalid) {
      ui_compositor_call_cursor_goto(row,col);
    }
    ui_compositor_call_put(str);
  }
  col++;
}

static void ui_compositor_eol_clear(UI *ui)
{
  static String wh = STATIC_CSTR_AS_STRING(" ");
  if (grid != float_grid && float_grid != NULL) {
    int save_col = col, save_row = row;
    while (col < grid->Columns) {
      ui_compositor_put(ui,wh);
    }
    col = save_col;
    row = save_row;
    // curinvalid = true; will be enough when we handle all grid events
    ui_compositor_grid_cursor_goto(ui, grid->handle, row, col);
  } else {
    ui_compositor_call_eol_clear();
  }
}

static void ui_compositor_resize(UI *ui, Integer rows, Integer columns)
{
  if (grid->handle == 1) {
    ui_compositor_call_resize(rows, columns);
  }
}

