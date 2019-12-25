#ifndef NVIM_MARK_EXTENDED_H
#define NVIM_MARK_EXTENDED_H

#include "nvim/mark_extended_defs.h"
#include "nvim/buffer_defs.h"  // for buf_T

EXTERN int extmark_splice_pending INIT(= 0);

typedef struct
{
  uint64_t ns_id;
  uint64_t mark_id;
  int row;
  colnr_T col;
} ExtmarkInfo;

typedef kvec_t(ExtmarkInfo) ExtmarkArray;


// Undo/redo extmarks

typedef enum {
  kExtmarkNOOP,        // Extmarks shouldn't be moved
  kExtmarkUndo,        // Operation should be reversable/undoable
  kExtmarkNoUndo,      // Operation should not be reversable
  kExtmarkUndoNoRedo,  // Operation should be undoable, but not redoable
} ExtmarkOp;

// delete the columns between mincol and endcol
typedef struct {
  int start_row;
  colnr_T start_col;
  int oldextent_row;
  colnr_T oldextent_col;
  int newextent_row;
  colnr_T newextent_col;
} ExtmarkSplice;

// adjust linenumbers after :move operation
typedef struct {
  linenr_T line1;
  linenr_T line2;
  linenr_T last_line;
  linenr_T dest;
  linenr_T num_lines;
  linenr_T extra;
} AdjustMove;

// TODO(bfredl): reconsider if we really should track mark creation/updating
// itself, these are not really "edit" operation.
// extmark was created
typedef struct {
  uint64_t ns_id;
  uint64_t mark_id;
  int row;
  colnr_T col;
} ExtmarkSet;

// extmark was updated
typedef struct {
  uint64_t ns_id;
  uint64_t mark_id;
  int old_row;
  colnr_T old_col;
  int row;
  colnr_T col;
} ExtmarkUpdate;

// also used as part of :move operation? probably can be simplified to one
// event.
typedef struct {
  linenr_T l_lnum;
  colnr_T l_col;
  linenr_T u_lnum;
  colnr_T u_col;
  linenr_T p_lnum;
  colnr_T p_col;
} ExtmarkCopyPlace;

// extmark was cleared.
// TODO(bfredl): same reconsideration as for ExtmarkSet/ExtmarkUpdate
typedef struct {
  uint64_t ns_id;
  linenr_T l_lnum;
  linenr_T u_lnum;
} ExtmarkClear;


typedef enum {
  kSplice,
  kAdjustMove,
  kExtmarkSet,
  kExtmarkDel,
  kExtmarkUpdate,
  kExtmarkCopy,
  kExtmarkCopyPlace,
  kExtmarkClear,
} UndoObjectType;

// TODO(bfredl): reduce the number of undo action types
struct undo_object {
  UndoObjectType type;
  union {
    ExtmarkSplice splice;
    AdjustMove move;
    ExtmarkSet set;
    ExtmarkUpdate update;
    ExtmarkInfo copy;
    ExtmarkCopyPlace copy_place;
    ExtmarkClear clear;
  } data;
};


// For doing move of extmarks in substitutions
typedef struct {
  lpos_T startpos;
  lpos_T endpos;
  linenr_T lnum;
  int sublen;
} ExtmarkSubSingle;

// For doing move of extmarks in substitutions
typedef struct {
  lpos_T startpos;
  lpos_T endpos;
  linenr_T lnum;
  linenr_T newline_in_pat;
  linenr_T newline_in_sub;
  linenr_T lnum_added;
  lpos_T cm_start;  // start of the match
  lpos_T cm_end;    // end of the match
  int eol;    // end of the match
} ExtmarkSubMulti;

typedef kvec_t(ExtmarkSubSingle) extmark_sub_single_vec_t;
typedef kvec_t(ExtmarkSubMulti) extmark_sub_multi_vec_t;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_MARK_EXTENDED_H
