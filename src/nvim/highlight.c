// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// highlight.c: low level code for both UI, syntax and :terminal highlighting

#include "nvim/vim.h"
#include "nvim/highlight.h"
#include "nvim/highlight_defs.h"
#include "nvim/map.h"
#include "nvim/screen.h"
#include "nvim/syntax.h"
#include "nvim/ui.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/helpers.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "highlight.c.generated.h"
#endif

/// An attribute number is the index in attr_entries plus ATTR_OFF.
#define ATTR_OFF 1


static kvec_t(HlEntry) attr_entries = KV_INITIAL_VALUE;

static Map(HlEntry, int) *attr_entry_ids;
static Map(int, int) *combine_attr_entries;

void highlight_init(void)
{
  attr_entry_ids = map_new(HlEntry, int)();
  combine_attr_entries = map_new(int, int)();
}

static int get_attr_entry(HlEntry entry)
{
  int id = map_get(HlEntry, int)(attr_entry_ids, entry);
  if (id > 0) {
    return id;
  }

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

  id = (int)kv_size(attr_entries)+ATTR_OFF;
  kv_push(attr_entries, entry);

  map_put(HlEntry, int)(attr_entry_ids, entry, id);

  Dictionary inspect = hl_inspect(id);
  ui_call_hl_attr_set(id, entry.attr, inspect);
  api_free_dictionary(inspect);
  return id;
}

int hl_get_syn_attr(int idx, HlAttrs at_en) {
  // TODO: unconditional!
  if (at_en.cterm_fg_color != 0 || at_en.cterm_bg_color != 0
      || at_en.rgb_fg_color != -1 || at_en.rgb_bg_color != -1
      || at_en.rgb_sp_color != -1 || at_en.cterm_ae_attr != 0
      || at_en.rgb_ae_attr != 0) {

    // Fubbit, with new semantics we can update the table in place
    return get_attr_entry((HlEntry){.attr=at_en, .kind = kHlSyntax, .id1 = idx, .id2 = 0});
  } else {
    // If all the fields are cleared, clear the attr field back to default value
    return 0;
  }
}

int hl_get_ui_attr(int idx, int final_id)
{
  HlAttrs attrs = HLATTRS_INIT;

  int syn_attr = syn_id2attr(final_id);
  if (syn_attr != 0) {
    HlAttrs *aep = syn_cterm_attr2entry(syn_attr);
    if (aep) {
      attrs = *aep;
    }
  }
  return get_attr_entry((HlEntry){.attr=attrs, .kind = kHlUI, .id1 = idx, .id2 = final_id});

}

void update_window_hl(win_T *wp, bool invalid)
{
  if (!wp->w_hl_needs_update && !invalid) {
    return;
  }
  wp->w_hl_needs_update = false;

  // determine window specific background set in 'winhighlight'
  if (wp != curwin && wp->w_hl_ids[HLF_INACTIVE] > 0) {
    wp->w_hl_attr_normal = hl_get_ui_attr(HLF_INACTIVE, wp->w_hl_ids[HLF_INACTIVE]);
  } else if (wp->w_hl_id_normal > 0) {
    wp->w_hl_attr_normal = hl_get_ui_attr(-1,wp->w_hl_id_normal);
  } else {
    wp->w_hl_attr_normal = 0;
  }
  if (wp != curwin) {
    wp->w_hl_attr_normal = hl_combine_attr(hl_attr(HLF_INACTIVE),
                                           wp->w_hl_attr_normal);
  }

  for (int hlf = 0; hlf < (int)HLF_COUNT; hlf++) {
    int attr;
    if (wp->w_hl_ids[hlf] > 0) {
      attr = hl_get_ui_attr(hlf, wp->w_hl_ids[hlf]);
    } else {
      attr = hl_attr(hlf);
    }
    if (wp->w_hl_attr_normal != 0) {
      attr = hl_combine_attr(wp->w_hl_attr_normal, attr);
    }
    wp->w_hl_attrs[hlf] = attr;
  }
}

/// Return the attr number for a set of colors and font.
/// Add a new entry to the term_attr_table, attr_table or gui_attr_table
/// if the combination is new.
/// @return 0 for error.
int get_term_attr_entry(HlAttrs *aep)
{
  return get_attr_entry((HlEntry){.attr=*aep, .kind = kHlTerminal, .id1 = 0, .id2 = 0});
}

// Clear all highlight tables.
void clear_hl_tables(bool reinit)
{
  kv_destroy(attr_entries);
  if (reinit) {
    kv_init(attr_entries);
    map_clear(HlEntry, int)(attr_entry_ids);
    map_clear(int, int)(combine_attr_entries);
  } else {
    map_free(HlEntry, int)(attr_entry_ids);
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

  id = get_attr_entry((HlEntry){.attr=new_en, .kind = kHlComibne, .id1 = char_attr, .id2 = prim_attr});
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

  return hlattrs2dict(aep, rgb ? kTrue : kFalse);
}

/// Converts an HlAttrs into Dictionary
///
/// @param[in] aep data to convert
/// @param use_rgb use 'gui*' settings if true, else resorts to 'cterm*'
Dictionary hlattrs2dict(const HlAttrs *aep, TriState use_rgb)
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


  else if (use_rgb != kFalse) {
    if (aep->rgb_fg_color != -1) {
      PUT(hl, "foreground", INTEGER_OBJ(aep->rgb_fg_color));
    }

    if (aep->rgb_bg_color != -1) {
      PUT(hl, "background", INTEGER_OBJ(aep->rgb_bg_color));
    }

    if (aep->rgb_sp_color != -1) {
      PUT(hl, "special", INTEGER_OBJ(aep->rgb_sp_color));
    }
    if (use_rgb == kNone) {
      if (cterm_normal_fg_color != aep->cterm_fg_color) {
        PUT(hl, "cterm_fg", INTEGER_OBJ(aep->cterm_fg_color - 1));
      }

      if (cterm_normal_bg_color != aep->cterm_bg_color) {
        PUT(hl, "cterm_bg", INTEGER_OBJ(aep->cterm_bg_color - 1));
      }
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

// TODO: make me a flat array of combines, they are associative after all...
Dictionary hl_inspect(int attr)
{
  Dictionary ret = ARRAY_DICT_INIT;
  if (attr < ATTR_OFF || attr >= (int)kv_size(attr_entries)+ATTR_OFF) {
    return ret;
  }

  HlEntry e = kv_A(attr_entries, attr-ATTR_OFF);
  switch (e.kind) {
    case kHlSyntax:
      PUT(ret, "kind", STRING_OBJ(cstr_to_string("syntax")));
      PUT(ret, "name", STRING_OBJ(cstr_to_string((char *)syn_id2name(e.id1))));
      break;

    case kHlUI:
      PUT(ret, "kind", STRING_OBJ(cstr_to_string("ui")));
      PUT(ret, "ui_name", STRING_OBJ(cstr_to_string(hlf_names[e.id1])));
      PUT(ret, "syn_name", STRING_OBJ(cstr_to_string((char *)syn_id2name(e.id2))));
      break;

    case kHlComibne:
      PUT(ret, "kind", STRING_OBJ(cstr_to_string("combine")));
      PUT(ret, "char", DICTIONARY_OBJ(hl_inspect(e.id1)));
      PUT(ret, "prim", DICTIONARY_OBJ(hl_inspect(e.id2)));
      break;

    case kHlTerminal:
      PUT(ret, "kind", STRING_OBJ(cstr_to_string("term")));
  }
  return ret;
}
