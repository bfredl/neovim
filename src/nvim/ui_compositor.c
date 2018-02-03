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
#include "nvim/log.h"
#include "nvim/main.h"
#include "nvim/vim.h"
#include "nvim/ui.h"
#include "nvim/memory.h"
#include "nvim/ui_compositor.h"
#include "nvim/ugrid.h"
#include "nvim/syntax.h"
#include "nvim/api/private/helpers.h"

// TODO: only for debug
#include "nvim/os/os.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "ui_compositor.c.generated.h"
#endif

#define UI(b) (((UICompositorData *)b)->ui)


int composed_uis = 0;

static UI *compositor = NULL;

static ScreenGrid *curgrid;

kvec_t(ScreenGrid *) layers = KV_INITIAL_VALUE;
static size_t cur_index;

/// TODO: separate "our" cursor from the incoming cursor
static int cursor_row,cursor_col;
bool cursor_invalid;
bool scroll_native = false;
int scroll_top, scroll_bot, scroll_left, scroll_right;

bool debug_recompose = false;

void compositor_init(void) {
  if (compositor != NULL) {
    return;
  }
  compositor = xcalloc(1, sizeof(UI));

  compositor->rgb = true;
  //compositor->stop = compositor_stop;
  compositor->resize = compositor_resize;
  compositor->clear = compositor_clear;
  compositor->eol_clear = compositor_eol_clear;
  compositor->set_scroll_region = compositor_set_scroll_region;
  compositor->scroll = compositor_scroll;
  compositor->highlight_set = compositor_highlight_set;
  compositor->put = compositor_put;
  //compositor->update_fg = compositor_update_fg;
  //compositor->update_bg = compositor_update_bg;
  //compositor->update_sp = compositor_update_sp;
  //compositor->flush = compositor_flush;
  //compositor->option_set = compositor_option_set;
  compositor->grid_cursor_goto = compositor_grid_cursor_goto;
  //compositor.float_info = compositor_float_info;
  //compositor.float_close = compositor_float_close;

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
  ui->composed = false;
}

bool compositor_active(void) {
  return composed_uis != 0;
}

void compositor_put_grid(ScreenGrid *grid, int rowpos, int colpos, bool valid)
{
  for (size_t i = 0; i < kv_size(layers); i++) {
    ScreenGrid *layer = kv_A(layers, i);
    if (kv_A(layers,i) != grid) {
      continue;
    }
    if (rowpos != layer->comp_row || colpos != layer->comp_col) {
      int save_row = cursor_row, save_col = cursor_col;
      grid_clear(grid);
      grid->comp_row = rowpos;
      grid->comp_col = colpos;
      if (valid) {
        grid_draw(grid);
      }
      move_cursor(save_row, save_col);
    }
    return;
  }
  // not found: new grid
  kv_push(layers, grid);
  grid->comp_row = rowpos;
  grid->comp_col = colpos;
}

void compositor_remove_grid(ScreenGrid *grid)
{
  assert(curgrid != grid);
  for (size_t i = 0; i < kv_size(layers); i++) {
    if (kv_A(layers,i) != grid) {
      continue;
    }

    int save_row = cursor_row, save_col = cursor_col;
    grid_clear(grid);
    move_cursor(save_row, save_col);
    size_t after = kv_size(layers)-i-1;
    if (after > 0) {
      if (cur_index > i) {
        cur_index--;
      }
      memmove(&kv_A(layers,i),&kv_A(layers,i+1),sizeof(ScreenGrid *)*after);
    }
    (void)kv_pop(layers);
    return;
  }
  abort();
}

void compositor_set_grid(ScreenGrid *grid)
{
  curgrid = grid;
  for (size_t i = 0; i < kv_size(layers); i++) {
    if (kv_A(layers, i) == grid) {
      cur_index = i;
      return;
    }
  }
  abort();
}

static void compositor_grid_cursor_goto(UI *ui, Integer grid_handle, Integer r, Integer c)
{
  assert(curgrid->handle == grid_handle);
  move_cursor(curgrid->comp_row+(int)r, curgrid->comp_col+(int)c);
}

static void move_cursor(Integer r, Integer c) {
  cursor_row = (int)r;
  cursor_col = (int)c;
  // TODO: this should be assured downstream instead
  if (cursor_col < default_grid.Columns && cursor_row < default_grid.Rows) {
    ui_composed_call_cursor_goto(cursor_row, cursor_col);
    cursor_invalid = false;
  }
}

static int last_attr = 0;

static void compositor_highlight_set(UI *ui, HlAttrs attrs)
{
  // TODO: missmatch :(
  last_attr = -1;
  // only supports rgb...
  // also be lazy: right now we spam composited output with unused highlights
  ui_composed_call_highlight_set(attrs);
}

static void compositor_put(UI *ui, String str)
{
  for (size_t i = cur_index+1; i < kv_size(layers); i++) {
    ScreenGrid *grid = kv_A(layers, i);
    if (grid->Rows > 0
        && grid->comp_col <= cursor_col
        && cursor_col < grid->comp_col + grid->Columns
        && grid->comp_row <= cursor_row
    && cursor_row < grid->comp_row + grid->Rows) {
      cursor_col++;
      cursor_invalid = true;
      return;
    }
  }

  if (cursor_invalid) {
    ui_composed_call_cursor_goto(cursor_row,cursor_col);
  }
  ui_composed_call_put(str);
  cursor_col++;
}

static void compositor_clear(UI *ui)
{
  if (curgrid->handle == 1) {
    // TODO: optimize for the common case: all grids being
    // cleared in the same flush event
    ui_composed_call_clear();
    for (size_t i = cur_index+1; i < kv_size(layers); i++) {
      grid_draw(kv_A(layers, i));
    }
  } else {
    grid_draw(curgrid);
  }
}

static void compositor_eol_clear(UI *ui)
{
  String wh = STATIC_CSTR_AS_STRING(" ");
  // fubbit, check if actually necessary
  if (kv_size(layers) > 1) {
    int save_col = cursor_col, save_row = cursor_row;
    while (cursor_col < curgrid->Columns) {
      compositor_put(ui,wh);
    }
    move_cursor(save_row, save_col);
  } else {
    ui_composed_call_eol_clear();
  }
}

static void compositor_set_scroll_region(UI *ui, Integer top, Integer bot,
                                  Integer left, Integer right)
{
  bool fullscreen = false;
  if (!fullscreen && kv_size(layers) > cur_index+1) {
    scroll_top = (int)top;
    scroll_bot = (int)bot;
    scroll_left = (int)left;
    scroll_right = (int)right;
    scroll_native = false;
  } else {
    top += curgrid->comp_row;
    bot += curgrid->comp_row;
    left += curgrid->comp_col;
    right += curgrid->comp_col;
    scroll_native = true;
    ui_composed_call_set_scroll_region(top, bot, left, right);
  }
}

static void compositor_scroll(UI *ui, Integer count)
{
  if (scroll_native) {
    ui_composed_call_scroll(count);
  } else {
    // Fubbit:
    // 1. don't redraw the scrolled-in area
    // 2. check if rectangles actually overlap
    for (size_t i = cur_index; i < kv_size(layers); i++) {
      grid_draw(kv_A(layers, i));
    }
  }
}

static void compositor_resize(UI *ui, Integer rows, Integer columns)
{
  if (curgrid->handle == 1) {
    ui_composed_call_resize(rows, columns);
  }
}

static void grid_clear(ScreenGrid *grid) {
  // TODO: be more effective for small moves?
  for (int r = 0; r < grid->Rows; r++) {
    for (int c = 0; c < grid->Columns; c++) {
      // if(cursor_invalid) blah blah
      move_cursor(grid->comp_row+r, grid->comp_col+c);
      grid_char(&default_grid, grid->comp_row+r, grid->comp_col+c);
    }
  }
}

static void grid_draw(ScreenGrid *grid) {
  for (int r = 0; r < grid->Rows; r++) {
    for (int c = 0; c < grid->Columns; c++) {
      // TODO: mask just like compositor_put
      // if(cursor_invalid) blah blah
      move_cursor(grid->comp_row+r, grid->comp_col+c);
      grid_char(grid, r, c);
    }
  }
}

static void grid_char(ScreenGrid *grid, int r, int c)
{
  /* Check for illegal values, just in case (could happen just after
   * resizing). */
  if (r>= grid->Rows || c>= grid->Columns)
    abort();

  size_t off = grid->LineOffset[r] + (size_t)c;
  int attr = grid->ScreenAttrs[off];
  if (last_attr != attr) {
    set_highlight_args(attr);
    last_attr = attr;
  }

  if (grid->ScreenLinesUC[off] != 0) {
    char buf[MB_MAXBYTES + 1];

    // Convert UTF-8 character to bytes and write it.
    buf[utfc_char2bytes(grid, (int)off, (char_u *)buf)] = NUL;
    ui_composed_call_put(cstr_as_string(buf));
    if (utf_ambiguous_width((int)grid->ScreenLinesUC[off])) {
      cursor_invalid = true;
    }
  } else {
    ui_composed_call_put((String){.data = (char *)grid->ScreenLines+off,
                                  .size = 1});
  }
}

static void set_highlight_args(int attr_code)
{
  HlAttrs attrs = HLATTRS_INIT;

  if (attr_code == 0) {
    goto end;
  }

  if (attr_code != 0) {
    HlAttrs *aep = syn_cterm_attr2entry(attr_code);
    if (aep) {
      attrs = *aep;
    }
  }

end:
  // when $NVIM_COMPOSITOR_DEBUG, invert recomposed regions to separate them
  // from redrawn regions
  if (debug_recompose) {
    attrs.rgb_fg_color = 0x00ffffff - (attrs.rgb_fg_color != -1 ? attrs.rgb_fg_color : normal_fg);
    attrs.rgb_bg_color = 0x00ffffff - (attrs.rgb_bg_color != -1 ? attrs.rgb_bg_color : normal_bg);
  }

  ui_composed_call_highlight_set(attrs);
}


