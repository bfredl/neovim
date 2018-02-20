// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// highlight.c: low level code for both UI, syntax and :terminal highlighting

#include "nvim/vim.h"
#include "nvim/highlight.h"
#include "nvim/highlight_defs.h"
#include "nvim/map.h"
#include "nvim/screen.h"
#include "nvim/syntax.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/helpers.h"

typedef enum {
  kHlUI,
  kHlSyntax,
  kHlTerminal,
  kHlComibne,
} HlKind;

typedef struct {
  HlAttrs attr;
  HlKind kind;
  int id1;
  int id2;
} HlEntry;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "highlight.c.generated.h"
#endif

/// An attribute number is the index in attr_entries plus ATTR_OFF.
#define ATTR_OFF 1


static kvec_t(HlEntry) attr_entries = KV_INITIAL_VALUE;

static Map(HlAttrs, int) *term_attr_entries;
static Map(int, int) *combine_attr_entries;

void highlight_init(void)
{
  term_attr_entries = map_new(HlAttrs, int)();
  combine_attr_entries = map_new(int, int)();
}

static int put_attr_entry(HlEntry entry)
{
  static bool recursive = false;
  if (kv_size(attr_entries) + ATTR_OFF > MAX_TYPENR) {
    /*
     * Running out of attribute entries!  remove all attributes, and
     * compute new ones for all groups.
     * When called recursively, we are really out of numbers.
     */
    if (recursive) {
      EMSG(_("E424: Too many different highlighting attributes in use"));
      return 0;
    }
    recursive = true;

    clear_hl_tables(true);

    must_redraw = CLEAR;

    highlight_attr_set_all();

    recursive = false;
    if (entry.kind == kHlComibne) {
      // This entry is now invalid, don't put it
      return 0;
    }
  }

  int id = kv_size(attr_entries);
  kv_push(attr_entries, entry);

  return id+ATTR_OFF;
}

void hl_update_attr(int idx, int *id, HlAttrs at_en) {
  // TODO: unconditional!
  if (at_en.cterm_fg_color != 0 || at_en.cterm_bg_color != 0
      || at_en.rgb_fg_color != -1 || at_en.rgb_bg_color != -1
      || at_en.rgb_sp_color != -1 || at_en.cterm_ae_attr != 0
      || at_en.rgb_ae_attr != 0) {

    // Fubbit, with new semantics we can update the table in place
    *id = put_attr_entry((HlEntry){.attr=at_en, .kind = kHlSyntax, .id1 = idx, .id2 = 0});
  } else {
    // If all the fields are cleared, clear the attr field back to default value
    *id = 0;
  }
}
/// Return the attr number for a set of colors and font.
/// Add a new entry to the term_attr_table, attr_table or gui_attr_table
/// if the combination is new.
/// @return 0 for error.
int get_term_attr_entry(HlAttrs *aep)
{
  int id = map_get(HlAttrs, int)(term_attr_entries, *aep);
  if (id > 0) {
    return id;
  }

  id = put_attr_entry((HlEntry){.attr=*aep, .kind = kHlTerminal, .id1 = 0, .id2 = 0});
  if (id > 0) {
    map_put(HlAttrs, int)(term_attr_entries, *aep, id);
  }
  return id;
}

// Clear all highlight tables.
void clear_hl_tables(bool reinit)
{
  kv_destroy(attr_entries);
  if (reinit) {
    kv_init(attr_entries);
    map_clear(HlAttrs, int)(term_attr_entries);
    map_clear(int, int)(combine_attr_entries);
  } else {
    map_free(HlAttrs, int)(term_attr_entries);
    map_free(int, int)(combine_attr_entries);
  }
}

// Combine special attributes (e.g., for spelling) with other attributes
// (e.g., for syntax highlighting).
// "prim_attr" overrules "char_attr".
// This creates a new group when required.
// Since we expect there to be few spelling mistakes we don't cache the
// result.
// Return the resulting attributes.
int hl_combine_attr(int char_attr, int prim_attr)
{
  if (char_attr == 0) {
    return prim_attr;
  }

  if (prim_attr == 0) {
    return char_attr;
  }

  // TODO FIXME XXX
  int combine_tag = (char_attr << 16) + prim_attr;
  int id = map_get(int, int)(combine_attr_entries, combine_tag);
  if (id > 0) {
    return id;
  }

  HlAttrs *char_aep = NULL;
  HlAttrs *spell_aep;
  HlAttrs new_en = HLATTRS_INIT;


  // Find the entry for char_attr
  char_aep = syn_cterm_attr2entry(char_attr);

  if (char_aep != NULL) {
    // Copy all attributes from char_aep to the new entry
    new_en = *char_aep;
  }

  spell_aep = syn_cterm_attr2entry(prim_attr);
  if (spell_aep != NULL) {
    new_en.cterm_ae_attr |= spell_aep->cterm_ae_attr;
    new_en.rgb_ae_attr |= spell_aep->rgb_ae_attr;

    if (spell_aep->cterm_fg_color > 0) {
      new_en.cterm_fg_color = spell_aep->cterm_fg_color;
    }

    if (spell_aep->cterm_bg_color > 0) {
      new_en.cterm_bg_color = spell_aep->cterm_bg_color;
    }

    if (spell_aep->rgb_fg_color >= 0) {
      new_en.rgb_fg_color = spell_aep->rgb_fg_color;
    }

    if (spell_aep->rgb_bg_color >= 0) {
      new_en.rgb_bg_color = spell_aep->rgb_bg_color;
    }

    if (spell_aep->rgb_sp_color >= 0) {
      new_en.rgb_sp_color = spell_aep->rgb_sp_color;
    }
  }

  id = put_attr_entry((HlEntry){.attr=new_en, .kind = kHlComibne, .id1 = char_attr, .id2 = prim_attr});
  if (id > 0) {
    map_put(int, int)(combine_attr_entries, combine_tag, id);
  }

  return id;
}

/// \note this function does not apply exclusively to cterm attr contrary
/// to what its name implies
/// \warn don't call it with attr 0 (i.e., the null attribute)
HlAttrs *syn_cterm_attr2entry(int attr)
{
  attr -= ATTR_OFF;
  if (attr >= (int)kv_size(attr_entries)) {
    // did ":syntax clear"
    return NULL;
  }
  return &(kv_A(attr_entries, attr).attr);
}

/// Gets highlight description for id `attr_id` as a map.
Dictionary hl_get_attr_by_id(Integer attr_id, Boolean rgb, Error *err)
{
  HlAttrs *aep = NULL;
  Dictionary dic = ARRAY_DICT_INIT;

  if (attr_id == 0) {
    return dic;
  }

  aep = syn_cterm_attr2entry((int)attr_id);
  if (!aep) {
    api_set_error(err, kErrorTypeException,
                  "Invalid attribute id: %" PRId64, attr_id);
    return dic;
  }

  return hlattrs2dict(aep, rgb);
}

/// Converts an HlAttrs into Dictionary
///
/// @param[in] aep data to convert
/// @param use_rgb use 'gui*' settings if true, else resorts to 'cterm*'
Dictionary hlattrs2dict(const HlAttrs *aep, bool use_rgb)
{
  assert(aep);
  Dictionary hl = ARRAY_DICT_INIT;
  int mask  = use_rgb ? aep->rgb_ae_attr : aep->cterm_ae_attr;

  if (mask & HL_BOLD) {
    PUT(hl, "bold", BOOLEAN_OBJ(true));
  }

  if (mask & HL_STANDOUT) {
    PUT(hl, "standout", BOOLEAN_OBJ(true));
  }

  if (mask & HL_UNDERLINE) {
    PUT(hl, "underline", BOOLEAN_OBJ(true));
  }

  if (mask & HL_UNDERCURL) {
    PUT(hl, "undercurl", BOOLEAN_OBJ(true));
  }

  if (mask & HL_ITALIC) {
    PUT(hl, "italic", BOOLEAN_OBJ(true));
  }

  if (mask & (HL_INVERSE | HL_STANDOUT)) {
    PUT(hl, "reverse", BOOLEAN_OBJ(true));
  }


  if (use_rgb) {
    if (aep->rgb_fg_color != -1) {
      PUT(hl, "foreground", INTEGER_OBJ(aep->rgb_fg_color));
    }

    if (aep->rgb_bg_color != -1) {
      PUT(hl, "background", INTEGER_OBJ(aep->rgb_bg_color));
    }

    if (aep->rgb_sp_color != -1) {
      PUT(hl, "special", INTEGER_OBJ(aep->rgb_sp_color));
    }
  } else {
    if (cterm_normal_fg_color != aep->cterm_fg_color) {
      PUT(hl, "foreground", INTEGER_OBJ(aep->cterm_fg_color - 1));
    }

    if (cterm_normal_bg_color != aep->cterm_bg_color) {
      PUT(hl, "background", INTEGER_OBJ(aep->cterm_bg_color - 1));
    }
  }

  return hl;
}

