// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// UI wrapper that sends requests to the UI thread.
// Used by the built-in TUI and libnvim-based UIs.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include "nvim/lib/kvec.h"
#include "nvim/log.h"
#include "nvim/main.h"
#include "nvim/ascii.h"
#include "nvim/vim.h"
#include "nvim/ui.h"
#include "nvim/memory.h"
#include "nvim/compositor.h"
#include "nvim/ugrid.h"
#include "nvim/screen.h"
#include "nvim/syntax.h"
#include "nvim/api/private/helpers.h"
#include "nvim/os/os.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "compositor.c.generated.h"
#endif

#define UI(b) (((UICompositorData *)b)->ui)


int composed_uis = 0;

static UI *compositor = NULL;

static ScreenGrid *curgrid;

kvec_t(ScreenGrid *) layers = KV_INITIAL_VALUE;

static size_t bufsize = 0;
static schar_T *linebuf;
static sattr_T *attrbuf;

bool debug_recompose = false;

#ifndef NDEBUG
static int chk_width = 0, chk_height = 0;
#endif

static bool valid_screen = true;
static bool msg_scroll_mode = false;
static int msg_first_invalid = 0;

void compositor_init(void)
{
  if (compositor != NULL) {
    return;
  }
  compositor = xcalloc(1, sizeof(UI));

  compositor->rgb = true;
  compositor->grid_resize = compositor_grid_resize;
  compositor->grid_clear = compositor_grid_clear;
  compositor->grid_scroll = compositor_grid_scroll;
  compositor->grid_cursor_goto = compositor_grid_cursor_goto;
  compositor->raw_line = compositor_raw_line;
  compositor->win_scroll_over_start = compositor_win_scroll_over_start;
  compositor->win_scroll_over_reset = compositor_win_scroll_over_reset;
  compositor->flush = compositor_flush;

  // Be unoptionated: will be attached together with a "real" ui anyway
  compositor->width = INT_MAX;
  compositor->height = INT_MAX;
  for (UIExtension i = 0; (int)i < kUIExtCount; i++) {
    compositor->ui_ext[i] = true;
  }

  kv_push(layers, &default_grid);
  curgrid = &default_grid;

  ui_attach_impl(compositor);

  if (os_getenv("NVIM_COMPOSITOR_DEBUG")) {
    debug_recompose = true;
  }
}


void compositor_attach(UI *ui)
{
  // compositor_init()
  composed_uis++;
  ui->composed = true;
}

void compositor_detach(UI *ui)
{
  composed_uis--;
  if (composed_uis == 0) {
    xfree(linebuf);
    xfree(attrbuf);
    linebuf = NULL;
    attrbuf = NULL;
    bufsize = 0;
  }
  ui->composed = false;
}

bool compositor_active(void)
{
  return composed_uis != 0;
}

void compositor_put_grid(ScreenGrid *grid, int rowpos, int colpos, bool valid)
{
  if (grid->comp_index != 0) {
    if (rowpos != grid->comp_row || colpos != grid->comp_col) {
      int old_row = grid->comp_row;
      // Temporalily mark grid as invisible
      grid->comp_row = default_grid.Rows;
      // grids will be resized and moved when the shell is resized but we
      // cant't draw right then as the screen is not valid. Floats will
      // instead be drawn after grid is cleared after the shell resize.
      if (valid_screen && compositor_active()) {
      // TODO(bfredl): inefficent, only draw up to grid->comp_index
        compose_area(old_row, old_row+grid->Rows, grid->comp_col,
                     grid->comp_col+grid->Columns);
      }
      grid->comp_row = rowpos;
      grid->comp_col = colpos;
      if (valid) {
        grid_draw(grid);
      }
    }
    return;
  }
#ifndef NDEBUG
  for (size_t i = 0; i < kv_size(layers); i++) {
    if (kv_A(layers, i) == grid) {
      assert(false);
    }
  }
#endif
  // not found: new grid
  kv_push(layers, grid);
  grid->comp_row = rowpos;
  grid->comp_col = colpos;
  grid->comp_index = kv_size(layers)-1;
  if (valid && valid_screen && compositor_active()) {
    grid_draw(grid);
  }
}

void compositor_remove_grid(ScreenGrid *grid)
{
  assert(grid != &default_grid);
  if (grid->comp_index == 0) {
    // grid wasn't present
    return;
  }

  if (curgrid == grid) {
    curgrid = &default_grid;
  }

  for (size_t i = grid->comp_index; i < kv_size(layers)-1; i++) {
    kv_A(layers, i) = kv_A(layers, i+1);
    kv_A(layers, i)->comp_index = i;
  }
  (void)kv_pop(layers);
  grid->comp_index = 0;

  if (compositor_active()) {
    // inefficent: only draw up to grid->comp_index
    compose_area(grid->comp_row, grid->comp_row+grid->Rows,
                 grid->comp_col, grid->comp_col+grid->Columns);
  }
}

bool compositor_set_grid(handle_T handle)
{
  if (curgrid->handle == handle) {
    return true;
  }
  ScreenGrid *grid = NULL;
  for (size_t i = 0; i < kv_size(layers); i++) {
    if (kv_A(layers, i)->handle == handle) {
      grid = kv_A(layers, i);
      break;
    }
  }
  if (grid != NULL) {
    curgrid = grid;
    return true;
  }
  return false;
}

static void compositor_grid_cursor_goto(UI *ui, Integer grid_handle,
                                        Integer r, Integer c)
{
  // TODO(bfred): avoid this lookup
  if (!compositor_active() || !compositor_set_grid((int)grid_handle)) {
    return;
  }
  int cursor_row = curgrid->comp_row+(int)r;
  int cursor_col = curgrid->comp_col+(int)c;

  if (cursor_col >= default_grid.Columns || cursor_row >= default_grid.Rows) {
    abort();
  }
  ui_composed_call_grid_cursor_goto(1, cursor_row, cursor_col);
}


/// BASELINE implementation. This is always correct, but we can almost
/// always do something more efficient (where efficiency means smaller
/// deltas to the downstream UI.)
static void compose_line(Integer row, Integer startcol, Integer endcol,
                         bool wrap)
{
  int col = (int)startcol;
  ScreenGrid *grid = NULL;

  while (col < endcol) {
    int until = 0;
    for (size_t i = 0; i < kv_size(layers); i++) {
      ScreenGrid *g = kv_A(layers, i);
      if (g->comp_row > row || row >= g->comp_row + g->Rows) {
        continue;
      }
      if (g->comp_col <= col && col < g->comp_col+g->Columns) {
        grid = g;
        until = g->comp_col+g->Columns;
      } else if (g->comp_col > col) {
        until = MIN(until, g->comp_col);
      }
    }
    assert(grid != NULL);
    assert(until > col);
    assert(until <= default_grid.Columns);
    size_t n = (size_t)(until-col);
    size_t off = grid->line_offset[row-grid->comp_row]
                 + (size_t)(col-grid->comp_col);
    memcpy(linebuf+(col-startcol), grid->chars+off, n * sizeof(*linebuf));
    memcpy(attrbuf+(col-startcol), grid->attrs+off, n * sizeof(*attrbuf));
    col = until;
  }
  assert(endcol <= chk_width);
  assert(row < chk_height);
  // TODO(bfredl): too conservative, need check
  // grid->line_wraps if grid->Width == Width
  wrap = wrap && grid && grid->handle == 1;
  ui_composed_call_raw_line(1, row, startcol, endcol, endcol, 0, wrap,
                            (const schar_T *)linebuf, (const sattr_T *)attrbuf);
}

static void compose_area(Integer startrow, Integer endrow,
                         Integer startcol, Integer endcol)
{
  for (int r = (int)startrow; r < endrow; r++) {
    compose_line(r, startcol, endcol, false);
  }
}


static void draw_line(ScreenGrid *grid, Integer row, Integer startcol,
                      Integer endcol, Integer clearcol, Integer clearattr,
                      bool wrap, const schar_T *chunk, const sattr_T *attrs)
{
  row += grid->comp_row;
  startcol += grid->comp_col;
  endcol += grid->comp_col;
  clearcol += grid->comp_col;
  wrap = wrap && grid->handle == 1;
  if (kv_size(layers) > grid->comp_index+1) {
    // NOT efficient
    compose_line(row, startcol, clearcol, wrap);
  } else {
    ui_composed_call_raw_line(1, row, startcol, endcol, clearcol, clearattr,
                              wrap, chunk, attrs);
  }
}

static void compositor_raw_line(UI *ui, Integer grid, Integer row,
                                Integer startcol, Integer endcol,
                                Integer clearcol, Integer clearattr, bool wrap,
                                const schar_T *chunk, const sattr_T *attrs)
{
  if (!compositor_active() || !compositor_set_grid((int)grid)) {
    return;
  }
  draw_line(curgrid, row, startcol, endcol, clearcol, clearattr, wrap, chunk,
            attrs);
}

static void compositor_flush(UI *ui)
{
  ui_composed_call_flush();
}

/// The screen is invalid and will soon be cleared
///
/// Don't redraw floats until screen is cleared
void compositor_invalidate_screen(void)
{
  valid_screen = false;
}

static void compositor_grid_clear(UI *ui, Integer grid)
{
  if (grid == 1) {
    // TODO(bfredl): maybe optimize for `:redraw!`, where all grids is being
    // cleared in the same flush event
    ui_composed_call_grid_clear(1);
    valid_screen = true;
    return;  // fubbit: screen.c will hand-hold this. We aren't suppose to
             // use CLEAR anyway.
    for (size_t i = curgrid->comp_index+1; i < kv_size(layers); i++) {
      grid_draw(kv_A(layers, i));
    }
  } else {
    // fubbit: we know that we will be overdrawn. Use damage regions
    // or just composed mode already. Or rather: just don't clear.
    if (!compositor_active() || !compositor_set_grid((int)grid)) {
      return;
    }
    grid_draw(curgrid);
  }
}

static void compositor_win_scroll_over_start(UI *ui)
{
  msg_scroll_mode = true;
  msg_first_invalid = ui->height;
}

static void compositor_win_scroll_over_reset(UI *ui)
{
  msg_scroll_mode = false;
  for (size_t i = 1; i < kv_size(layers); i++) {
    ScreenGrid *grid = kv_A(layers, i);
    if (grid->comp_row+grid->Rows > msg_first_invalid) {
      grid_draw(grid);
    }
  }
}

static void compositor_grid_scroll(UI *ui, Integer grid, Integer top,
                                   Integer bot, Integer left, Integer right,
                                   Integer rows, Integer cols)
{
  if (!compositor_active() || !compositor_set_grid((int)grid)) {
    return;
  }
  top += curgrid->comp_row;
  bot += curgrid->comp_row;
  left += curgrid->comp_col;
  right += curgrid->comp_col;
  if (!msg_scroll_mode && kv_size(layers) > curgrid->comp_index+1) {
    // Fubbit:
    // 1. don't redraw the scrolled-in area
    // 2. check if rectangles actually overlap
    compose_area(top, bot, left, right);
  } else {
    msg_first_invalid = MIN(msg_first_invalid, (int)top);
    ui_composed_call_grid_scroll(1, top, bot, left, right, rows, cols);
  }
}

static void compositor_grid_resize(UI *ui, Integer grid,
                                   Integer width, Integer height)
{
  // TODO(bfredl): compositor_active()
  if (grid == 1) {
    ui_composed_call_grid_resize(1, width, height);
#ifndef NDEBUG
    chk_width = (int)width;
    chk_height = (int)height;
#endif
    size_t new_bufsize = (size_t)width;
    if (bufsize != new_bufsize) {
      xfree(linebuf);
      xfree(attrbuf);
      linebuf = xmalloc(new_bufsize * sizeof(*linebuf));
      attrbuf = xmalloc(new_bufsize * sizeof(*attrbuf));
      bufsize = new_bufsize;
    }
  }
}

static void grid_draw(ScreenGrid *grid)
{
  for (int r = 0; r < grid->Rows; r++) {
    size_t off = grid->line_offset[r];
    draw_line(grid, r, 0, grid->Columns, grid->Columns, 0,
              grid->line_wraps[r], (const schar_T *)grid->chars+off,
              (const sattr_T *)grid->attrs+off);
  }
}

