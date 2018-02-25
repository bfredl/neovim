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

static bool was_ambiwidth = false;
static int cursor_row, cursor_col;
static int put_row, put_col;

static bool put_attr_pending = false;
static HlAttrs put_attr = HLATTRS_INIT;

static bool cached_compose = false;
static ScreenGrid *cached_compose_grid = NULL;
static bool cached_draw = false;
static int cached_until = -1;

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
  compositor->flush = compositor_flush;
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
  cached_until = -1;
  for (size_t i = 0; i < kv_size(layers); i++) {
    ScreenGrid *layer = kv_A(layers, i);
    if (kv_A(layers,i) != grid) {
      continue;
    }
    if (rowpos != layer->comp_row || colpos != layer->comp_col) {
      grid_clear(grid);
      grid->comp_row = rowpos;
      grid->comp_col = colpos;
      if (valid) {
        grid_draw(grid);
      }
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
  cached_until = -1;
  assert(curgrid != grid);
  for (size_t i = 0; i < kv_size(layers); i++) {
    if (kv_A(layers,i) != grid) {
      continue;
    }

    grid_clear(grid);
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
  int new_row = curgrid->comp_row+(int)r;
  int new_col = curgrid->comp_col+(int)c;
  if (new_row != put_row || new_col < put_col) {
    cached_until = -1;
  }
  put_row = new_row;
  put_col = new_col;
  move_cursor(put_row, put_col, true);
}

static void move_cursor(Integer r, Integer c, bool force) {
  if (cursor_row == r && cursor_col == c && !was_ambiwidth && !force) {
    return;
  }
  cursor_row = (int)r;
  cursor_col = (int)c;
  was_ambiwidth = false;

  // TODO: this should be assured downstream instead
  if (cursor_col < default_grid.Columns && cursor_row < default_grid.Rows) {
    ui_composed_call_cursor_goto(cursor_row, cursor_col);
  }
}

static int last_attr = 0;

static void compositor_highlight_set(UI *ui, HlAttrs attrs)
{
  last_attr = -1;
  put_attr = attrs;
  put_attr_pending = true;
}

static void compositor_put(UI *ui, String str)
{
  //String xx = STATIC_CSTR_AS_STRING("x");
  if (put_col >= cached_until) {
    //str = xx;
    cached_until = INT32_MAX;
    cached_draw = true;
    cached_compose = false;
    cached_compose_grid = curgrid;
    for (size_t i = cur_index+1; i < kv_size(layers); i++) {
      ScreenGrid *grid = kv_A(layers, i);
      if (grid->Rows > 0
          && grid->comp_row <= put_row
          && put_row < grid->comp_row + grid->Rows) {

          if (grid->comp_col <= put_col
              && put_col < grid->comp_col + grid->Columns) {
            if (true) {
              cached_compose = true;
              cached_compose_grid = kv_A(layers,i);
              cached_until = MIN(cached_until, grid->comp_col + grid->Columns);
            } else {
              cached_draw = false;
              cached_until = grid->comp_col + grid->Columns;
              break;
            }
          } else if (grid->comp_col > put_col) {
            cached_until = MIN(cached_until, grid->comp_col);
          }

      }
    }
    if (true && cur_index > 0) {
      cached_compose = true;
    }
  }
  if (cached_draw) {
    move_cursor(put_row, put_col, false);
    if (cached_compose) {
      grid_char_compose(cached_compose_grid, put_row-cached_compose_grid->comp_row, put_col-cached_compose_grid->comp_col);
      put_attr_pending = true;
    } else {
      if (put_attr_pending) {
        ui_composed_call_highlight_set(put_attr);
        put_attr_pending = false;
      }
      ui_composed_call_put(str);
    }
  }
  put_col++;
}

static void compositor_flush(UI *ui)
{
  move_cursor(put_row, put_col, false);
  ui_composed_call_flush();
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
    // fubbit: we know that we will be overdraw. Use damage regions
    // or just composed mode already.
    grid_draw(curgrid);
  }
}

static void compositor_eol_clear(UI *ui)
{
  String wh = STATIC_CSTR_AS_STRING(" ");
  // fubbit, check if actually necessary
  if (kv_size(layers) > 1) {
    int save_col = put_col, save_row = put_row;
    while (put_col < curgrid->Columns) {
      compositor_put(ui,wh);
    }
    put_col = save_col;
    put_row = save_row;
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
    move_cursor(grid->comp_row+r, grid->comp_col, false);
    for (int c = 0; c < grid->Columns; c++) {
      // if(cursor_invalid) blah blah
      grid_char(&default_grid, grid->comp_row+r, grid->comp_col+c);
    }
  }
}

static void grid_draw(ScreenGrid *grid) {
  for (int r = 0; r < grid->Rows; r++) {
    move_cursor(grid->comp_row+r, grid->comp_col, false);
    for (int c = 0; c < grid->Columns; c++) {
      // TODO: mask just like compositor_put
      // if(cursor_invalid) blah blah
      if (true && grid != &default_grid) {
        grid_char_compose(grid, r, c);
      } else {
        grid_char(grid, r, c);
      }
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
  if (grid->ScreenLines[off] == NUL) {
    cursor_col++;
    return;
  }

  int attr = grid->ScreenAttrs[off];
  if (last_attr != attr) {
    HlAttrs attrs = get_attrs(attr);

    // when $NVIM_COMPOSITOR_DEBUG, invert recomposed regions to separate them
    // from redrawn regions
    if (debug_recompose) {
      attrs.rgb_fg_color = 0x00ffffff - (attrs.rgb_fg_color != -1 ? attrs.rgb_fg_color : normal_fg);
      attrs.rgb_bg_color = 0x00ffffff - (attrs.rgb_bg_color != -1 ? attrs.rgb_bg_color : normal_bg);
    }

    ui_composed_call_highlight_set(attrs);
    last_attr = attr;
  }
  if (was_ambiwidth) {
    move_cursor(cursor_row, cursor_col, true);
  }

  if (grid->ScreenLinesUC[off] != 0) {
    char buf[MB_MAXBYTES + 1];

    // Convert UTF-8 character to bytes and write it.
    buf[utfc_char2bytes(grid, (int)off, (char_u *)buf)] = NUL;
    ui_composed_call_put(cstr_as_string(buf));
    if (utf_ambiguous_width((int)grid->ScreenLinesUC[off])) {
      was_ambiwidth = true;
    }
    cursor_col++;
  } else {
    ui_composed_call_put((String){.data = (char *)grid->ScreenLines+off,
                                  .size = 1});
    cursor_col++;
  }
}

static HlAttrs get_attrs(int attr_code)
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
  if (attrs.rgb_bg_color == -1) {
    attrs.rgb_bg_color = normal_bg;
  }

  if (attrs.rgb_fg_color == -1) {
    attrs.rgb_fg_color = normal_fg;
  }



  return attrs;
}

static void grid_char_compose(ScreenGrid *grid, int r, int c)
{
  /* Check for illegal values, just in case (could happen just after
   * resizing). */
  if (r>= grid->Rows || c>= grid->Columns)
    abort();

  size_t off = grid->LineOffset[r] + (size_t)c;
  if (grid->ScreenLines[off] == NUL) {
    cursor_col++;
    return;
  }

  HlAttrs gattrs = get_attrs(grid->ScreenAttrs[off]);

  size_t moff = default_grid.LineOffset[r+grid->comp_row] + (size_t)c + grid->comp_col;
  HlAttrs mattrs = get_attrs(default_grid.ScreenAttrs[moff]);
  HlAttrs cattrs;

  bool thru = (c > 0 && grid->ScreenLines[off-1] == ' ' && grid->ScreenLines[off] == ' ' && (c == grid->Columns-1 || grid->ScreenLines[off+1] == ' '));

  if (thru) {
    cattrs = mattrs;
    cattrs.rgb_fg_color = mix(gattrs.rgb_bg_color, mattrs.rgb_fg_color);
    grid = &default_grid;
    off = moff;
  } else {
    cattrs = gattrs;
    cattrs.rgb_fg_color = mix(gattrs.rgb_fg_color, mattrs.rgb_bg_color);
  }
  cattrs.rgb_bg_color = mix(gattrs.rgb_bg_color, mattrs.rgb_bg_color);

  ui_composed_call_highlight_set(cattrs);
  last_attr = -1;

  if (was_ambiwidth) {
    move_cursor(cursor_row, cursor_col, true);
  }

  if (grid->ScreenLinesUC[off] != 0) {
    char buf[MB_MAXBYTES + 1];

    // Convert UTF-8 character to bytes and write it.
    buf[utfc_char2bytes(grid, (int)off, (char_u *)buf)] = NUL;
    ui_composed_call_put(cstr_as_string(buf));
    if (utf_ambiguous_width((int)grid->ScreenLinesUC[off])) {
      was_ambiwidth = true;
    }
    cursor_col++;
  } else {
    ui_composed_call_put((String){.data = (char *)grid->ScreenLines+off,
                                  .size = 1});
    cursor_col++;
  }
}

static int mix(int front, int back) {
  float a = 0.66f;
  float b = 1.0f-a;
  int fr = (front & 0xFF0000) >> 16;
  int fg = (front & 0x00FF00) >> 8;
  int fb = (front & 0x0000FF) >> 0;
  int br = (back & 0xFF0000) >> 16;
  int bg = (back & 0x00FF00) >> 8;
  int bb = (back & 0x0000FF) >> 0;
  int mr = (int)(a*fr+b*br);
  int mg = (int)(a*fg+b*bg);
  int mb = (int)(a*fb+b*bb);
  return (mr << 16) + (mg << 8) + mb;
}
