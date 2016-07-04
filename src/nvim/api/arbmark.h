#ifndef NVIM_ARBMARK_H
#define NVIM_ARBMARK_H

#include "nvim/buffer.h"
#include "nvim/mark.h"
#include "nvim/map.h"

#define ARBMARK_MAXLEN 24
#define pos_cmp(a, b) (_pos_cmp((a).fmark.mark, (b).fmark.mark))

typedef struct {
  fmark_T fmark;
  fmark_T *next;
  fmark_T *prev;
} arbmark_T;

int arbmark_set(buf_T *buf, char_u *name, pos_T *pos);
int arbmark_unset(buf_T *buf, char_u *name);
arbmark_T *arbmark_next(pos_T pos, int fnum);
arbmark_T *arbmark_prev(pos_T pos, int fnum);
kvec_t(cstr_t) arbmark_names(char_u *name);
int _pos_cmp(pos_T a, pos_T b);
buf_T *arbmark_buf_from_fnum(int fnum);

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "arbmark.h.generated.h"
#endif
#endif  // NVIM_ARBMARK_H
