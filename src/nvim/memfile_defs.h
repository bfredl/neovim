#ifndef NVIM_MEMFILE_DEFS_H
#define NVIM_MEMFILE_DEFS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "nvim/map.h"
#include "nvim/pos.h"
#include "nvim/types.h"

#define BH_DIRTY    1U
#define BH_LOCKED   2U

typedef enum {
  MF_DIRTY_NO = 0,      ///< no dirty blocks
  MF_DIRTY_YES,         ///< there are dirty blocks
  MF_DIRTY_YES_NOSYNC,  ///< there are dirty blocks, do not sync yet
} mfdirty_T;

/// A memory file.
typedef struct memfile {
  char *mf_fname;                    ///< name of the file
  char *mf_ffname;                   ///< idem, full path
  int mf_fd;                         ///< file descriptor

  /// The used blocks are kept in mf_hash.
  /// mf_hash are used to quickly find a block in the used list.
  Set(bhdr_T) mf_hash;

  // list of free blocks. These have no memory allocated, but they
  // have reserved space in the swapfile, if any
  kvec_t(bhdr_T) mf_free;

  /// When a block with a negative number is flushed to the file, it gets
  /// a positive number. Because the reference to the block is still the negative
  /// number, we remember the translation to the new positive number.
  Map(int64_t, int64_t) mf_trans;

  blocknr_T mf_blocknr_max;          ///< highest positive block number + 1
  blocknr_T mf_blocknr_min;          ///< lowest negative block number - 1
  blocknr_T mf_neg_count;            ///< number of negative blocks numbers
  blocknr_T mf_infile_count;         ///< number of pages in the file
  unsigned mf_page_size;             ///< number of bytes in a page
  mfdirty_T mf_dirty;
} memfile_T;

#endif  // NVIM_MEMFILE_DEFS_H
