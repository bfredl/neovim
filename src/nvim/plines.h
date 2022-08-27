#ifndef NVIM_PLINES_H
#define NVIM_PLINES_H

#include "nvim/vim.h"


// Argument for lbr_chartabsize().
typedef struct {
  win_T *cts_win;
  linenr_T cts_lnum;   // zero when not using text properties
  char *cts_line;    // start of the line
  char *cts_ptr;     // current position in line

  bool cts_has_prop_with_text;  // TRUE if if a property inserts text
  int cts_cur_text_width;     // width of current inserted text
  /*
  int		cts_text_prop_count;	// number of text props
  textprop_T	*cts_text_props;	// text props (allocated) or NULL
  */

  int cts_vcol;    // virtual column at current position
} chartabsize_T;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "plines.h.generated.h"
#endif
#endif  // NVIM_PLINES_H
