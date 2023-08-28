#ifndef NVIM_MEMLINE_H
#define NVIM_MEMLINE_H

#include "nvim/buffer_defs.h"
#include "nvim/pos.h"
#include "nvim/types.h"

EXTERN bool process_still_running INIT(= false);

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "memline.h.generated.h"
#endif
#endif  // NVIM_MEMLINE_H
