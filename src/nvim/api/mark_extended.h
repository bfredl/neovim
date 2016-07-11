#ifndef NVIM_EXTMARK_H
#define NVIM_EXTMARK_H

#include "nvim/mark.h"

#define EXTMARK_MAXLEN 24
#define pos_cmp(a, b) (_pos_cmp((a).fmark.mark, (b).fmark.mark))

typedef struct {
  fmark_T fmark;
  fmark_T *next;
  fmark_T *prev;
} ExtendedMark;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "api/extmark.h.generated.h"
#endif

#endif  // NVIM_EXTMARK_H
