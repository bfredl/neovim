// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "nvim/vim.h"
#include "nvim/log.h"
#include "nvim/map.h"
#include "nvim/ui_client.h"
#include "nvim/api/private/helpers.h"
#include "nvim/msgpack_rpc/channel.h"
#include "nvim/api/private/dispatch.h"
#include "nvim/ui.h"
#include "nvim/highlight.h"
#include "nvim/screen.h"

static Map(String, UIClientHandler) ui_client_handlers = MAP_INIT;

// Temporary buffer for converting a single grid_line event
static size_t buf_size = 0;
static schar_T *buf_char = NULL;
static sattr_T *buf_attr = NULL;

static void add_ui_client_event_handler(String method, UIClientHandler handler)
{
  map_put(String, UIClientHandler)(&ui_client_handlers, method, handler);
}

void ui_client_init(uint64_t chan)
{
  Array args = ARRAY_DICT_INIT;
  int width = Columns;
  int height = Rows;
  Dictionary opts = ARRAY_DICT_INIT;

  PUT(opts, "rgb", BOOLEAN_OBJ(true));
  PUT(opts, "ext_linegrid", BOOLEAN_OBJ(true));
  PUT(opts, "ext_termcolors", BOOLEAN_OBJ(true));

  ADD(args, INTEGER_OBJ((int)width));
  ADD(args, INTEGER_OBJ((int)height));
  ADD(args, DICTIONARY_OBJ(opts));

  rpc_send_event(chan, "nvim_ui_attach", args);
  msgpack_rpc_add_redraw();  // GAME!
  // TODO(bfredl): use a keyset instead
  ui_client_methods_table_init();
  ui_client_channel_id = chan;
}

/// Handler for "redraw" events sent by the NVIM server
///
/// This function will be called by handle_request (in msgpack_rpc/channel.c)
/// The individual ui_events sent by the server are individually handled
/// by their respective handlers defined in ui_events_client.generated.h
///
/// @note The "flush" event is called only once and only after handling all
///       the other events
/// @param channel_id: The id of the rpc channel
/// @param uidata: The dense array containing the ui_events sent by the server
/// @param[out] err Error details, if any
Object ui_client_handle_redraw(uint64_t channel_id, Array args, Error *error)
{
  for (size_t i = 0; i < args.size; i++) {
    Array call = args.items[i].data.array;
    String name = call.items[0].data.string;

    UIClientHandler handler = map_get(String, UIClientHandler)(&ui_client_handlers, name);
    if (!handler) {
      ELOG("No ui client handler for %s", name.size ? name.data : "<empty>");
      continue;
    }

    // fprintf(stderr, "%s: %zu\n", name.data, call.size-1);
    DLOG("Invoke ui client handler for %s", name.data);
    for (size_t j = 1; j < call.size; j++) {
      handler(call.items[j].data.array);
    }
  }

  return NIL;
}

/// run the main thread in ui client mode
///
/// This is just a stub. the full version will handle input, resizing, etc
void ui_client_execute(uint64_t chan)
{
  while (true) {
    loop_poll_events(&main_loop, -1);
  }

  getout(0);
}

static HlAttrs ui_client_dict2hlattrs(Dictionary d, bool rgb)
{
  Error err = ERROR_INIT;
  Dict(highlight) dict = { 0 };
  if (!api_dict_to_keydict(&dict, KeyDict_highlight_get_field, d, &err)) {
    // TODO(bfredl): log "err"
    return HLATTRS_INIT;
  }
  return dict2hlattrs(&dict, true, NULL, &err);
}

#ifdef INCLUDE_GENERATED_DECLARATIONS
#include "ui_events_client.generated.h"
#endif

void ui_client_event_grid_line(Array args)
{
  if (args.size < 4
      || args.items[0].type != kObjectTypeInteger
      || args.items[1].type != kObjectTypeInteger
      || args.items[2].type != kObjectTypeInteger
      || args.items[3].type != kObjectTypeArray) {
    goto error;
  }

  Integer grid = args.items[0].data.integer;
  Integer row = args.items[1].data.integer;
  Integer startcol = args.items[2].data.integer;
  Array cells = args.items[3].data.array;

  Integer endcol, clearcol, clearattr;
  // TODO(hlpr98): Accomodate other LineFlags when included in grid_line
  LineFlags lineflags = 0;
  size_t no_of_cells = cells.size;
  endcol = startcol;

  // checking if clearcol > endcol
  if (!STRCMP(cells.items[cells.size-1].data.array
              .items[0].data.string.data, " ")
      && cells.items[cells.size-1].data.array.size == 3) {
    no_of_cells = cells.size - 1;
  }

  // getting endcol
  for (size_t i = 0; i < no_of_cells; i++) {
    endcol++;
    if (cells.items[i].data.array.size == 3) {
      endcol += cells.items[i].data.array.items[2].data.integer - 1;
    }
  }

  if (!STRCMP(cells.items[cells.size-1].data.array
              .items[0].data.string.data, " ")
      && cells.items[cells.size-1].data.array.size == 3) {
    clearattr = cells.items[cells.size-1].data.array.items[1].data.integer;
    clearcol = endcol + cells.items[cells.size-1].data.array
                                                      .items[2].data.integer;
  } else {
    clearattr = 0;
    clearcol = endcol;
  }

  size_t ncells = (size_t)(endcol - startcol);
  if (buf_size < ncells) {
    xfree(buf_char);
    xfree(buf_attr);
    buf_char = xmalloc(ncells * sizeof(schar_T));
    buf_attr = xmalloc(ncells * sizeof(sattr_T));
    buf_size = ncells;
  }

  size_t j = 0;
  size_t k = 0;
  for (size_t i = 0; i < no_of_cells; i++) {
    if (cells.items[i].type != kObjectTypeArray) {
      goto error;
    }
    Array cell = cells.items[i].data.array;
    STRCPY(buf_char[j++], cell.items[0].data.string.data);
    if (cell.size == 3) {
      // repeat present
      for (size_t rep = 1; rep < (size_t)cell.items[2].data.integer; rep++) {
        STRCPY(buf_char[j++], cell.items[0].data.string.data);
        buf_attr[k++] = (sattr_T)cell.items[1].data.integer;
      }
    } else if (cell.size == 2) {
      // repeat = 1 but attrs != last_hl
      buf_attr[k++] = (sattr_T)cell.items[1].data.integer;
    }
    if (j > k) {
      // attrs == last_hl
      buf_attr[k] = buf_attr[k-1];
      k++;
    }
  }

  ui_call_raw_line(grid, row, startcol, endcol, clearcol, clearattr, lineflags,
                   buf_char, buf_attr);
  return;

error:
    ELOG("malformatted 'grid_line' event");
}
