// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "nvim/memline.h"
#include "nvim/eval.h"
#include "nvim/eval/typval.h"
#include "nvim/eval/typval_defs.h"
#include "nvim/fileio.h"
#include "nvim/option.h"
#include "nvim/swapfile.h"
#include "nvim/ui.h"
#include "nvim/vim.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "swapfile.c.generated.h"
#endif

/// Find the names of swap files in current directory and the directory given
/// with the 'directory' option.
///
/// Used to:
/// - list the swap files for "vim -r"
/// - count the number of swap files when recovering
/// - list the swap files when recovering
/// - list the swap files for swapfilelist()
/// - find the name of the n'th swap file when recovering
///
/// @param fname  base for swap file name
/// @param do_list  when true, list the swap file names
/// @param ret_list  when not NULL add file names to it
/// @param nr  when non-zero, return nr'th swap file name
/// @param fname_out  result when "nr" > 0
int recover_names(char *fname, bool do_list, list_T *ret_list, int nr, char **fname_out)
{
  int num_names;
  char *(names[6]);
  char *tail;
  char *p;
  int file_count = 0;
  char **files;
  char *fname_res = NULL;
#ifdef HAVE_READLINK
  char fname_buf[MAXPATHL];
#endif

  if (fname != NULL) {
#ifdef HAVE_READLINK
    // Expand symlink in the file name, because the swap file is created
    // with the actual file instead of with the symlink.
    if (resolve_symlink(fname, fname_buf) == OK) {
      fname_res = fname_buf;
    } else
#endif
    fname_res = fname;
  }

  if (do_list) {
    // use msg() to start the scrolling properly
    msg(_("Swap files found:"));
    msg_putchar('\n');
  }

  // Do the loop for every directory in 'directory'.
  // First allocate some memory to put the directory name in.
  char *dir_name = xmalloc(strlen(p_dir) + 1);
  char *dirp = p_dir;
  while (*dirp) {
    // Isolate a directory name from *dirp and put it in dir_name (we know
    // it is large enough, so use 31000 for length).
    // Advance dirp to next directory name.
    (void)copy_option_part(&dirp, dir_name, 31000, ",");

    if (dir_name[0] == '.' && dir_name[1] == NUL) {     // check current dir
      if (fname == NULL) {
        names[0] = xstrdup("*.sw?");
        // For Unix names starting with a dot are special.  MS-Windows
        // supports this too, on some file systems.
        names[1] = xstrdup(".*.sw?");
        names[2] = xstrdup(".sw?");
        num_names = 3;
      } else {
        num_names = recov_file_names(names, fname_res, true);
      }
    } else {                      // check directory dir_name
      if (fname == NULL) {
        names[0] = concat_fnames(dir_name, "*.sw?", true);
        // For Unix names starting with a dot are special.  MS-Windows
        // supports this too, on some file systems.
        names[1] = concat_fnames(dir_name, ".*.sw?", true);
        names[2] = concat_fnames(dir_name, ".sw?", true);
        num_names = 3;
      } else {
        int len = (int)strlen(dir_name);
        p = dir_name + len;
        if (after_pathsep(dir_name, p)
            && len > 1
            && p[-1] == p[-2]) {
          // Ends with '//', Use Full path for swap name
          tail = make_percent_swname(dir_name, fname_res);
        } else {
          tail = path_tail(fname_res);
          tail = concat_fnames(dir_name, tail, true);
        }
        num_names = recov_file_names(names, tail, false);
        xfree(tail);
      }
    }

    int num_files;
    if (num_names == 0) {
      num_files = 0;
    } else if (expand_wildcards(num_names, names, &num_files, &files,
                                EW_KEEPALL|EW_FILE|EW_SILENT) == FAIL) {
      num_files = 0;
    }

    // When no swap file found, wildcard expansion might have failed (e.g.
    // not able to execute the shell).
    // Try finding a swap file by simply adding ".swp" to the file name.
    if (*dirp == NUL && file_count + num_files == 0 && fname != NULL) {
      char *swapname = modname(fname_res, ".swp", true);
      if (swapname != NULL) {
        if (os_path_exists(swapname)) {
          files = xmalloc(sizeof(char *));
          files[0] = swapname;
          swapname = NULL;
          num_files = 1;
        }
        xfree(swapname);
      }
    }

    // Remove swapfile name of the current buffer, it must be ignored.
    // But keep it for swapfilelist().
    if (curbuf->b_ml.ml_mfp != NULL
        && (p = curbuf->b_ml.ml_mfp->mf_fname) != NULL
        && ret_list == NULL) {
      for (int i = 0; i < num_files; i++) {
        // Do not expand wildcards, on Windows would try to expand
        // "%tmp%" in "%tmp%file"
        if (path_full_compare(p, files[i], true, false) & kEqualFiles) {
          // Remove the name from files[i].  Move further entries
          // down.  When the array becomes empty free it here, since
          // FreeWild() won't be called below.
          xfree(files[i]);
          if (--num_files == 0) {
            xfree(files);
          } else {
            for (; i < num_files; i++) {
              files[i] = files[i + 1];
            }
          }
        }
      }
    }
    if (nr > 0) {
      file_count += num_files;
      if (nr <= file_count) {
        *fname_out = xstrdup(files[nr - 1 + num_files - file_count]);
        dirp = "";                        // stop searching
      }
    } else if (do_list) {
      if (dir_name[0] == '.' && dir_name[1] == NUL) {
        if (fname == NULL) {
          msg_puts(_("   In current directory:\n"));
        } else {
          msg_puts(_("   Using specified name:\n"));
        }
      } else {
        msg_puts(_("   In directory "));
        msg_home_replace(dir_name);
        msg_puts(":\n");
      }

      if (num_files) {
        for (int i = 0; i < num_files; i++) {
          // print the swap file name
          msg_outnum((long)++file_count);
          msg_puts(".    ");
          msg_puts(path_tail(files[i]));
          msg_putchar('\n');
          (void)swapfile_info(files[i]);
        }
      } else {
        msg_puts(_("      -- none --\n"));
      }
      ui_flush();
    } else if (ret_list != NULL) {
      for (int i = 0; i < num_files; i++) {
        char *name = concat_fnames(dir_name, files[i], true);
        tv_list_append_allocated_string(ret_list, name);
      }
    } else {
      file_count += num_files;
    }

    for (int i = 0; i < num_names; i++) {
      xfree(names[i]);
    }
    if (num_files > 0) {
      FreeWild(num_files, files);
    }
  }
  xfree(dir_name);
  return file_count;
}

/// Append the full path to name with path separators made into percent
/// signs, to dir. An unnamed buffer is handled as "" (<currentdir>/"")
char *make_percent_swname(const char *dir, const char *name)
  FUNC_ATTR_NONNULL_ARG(1)
{
  char *d = NULL;
  char *f = fix_fname(name != NULL ? name : "");
  if (f == NULL) {
    return NULL;
  }

  char *s = xstrdup(f);
  for (d = s; *d != NUL; MB_PTR_ADV(d)) {
    if (vim_ispathsep(*d)) {
      *d = '%';
    }
  }
  d = concat_fnames(dir, s, true);
  xfree(s);
  xfree(f);
  return d;
}


static int recov_file_names(char **names, char *path, int prepend_dot)
  FUNC_ATTR_NONNULL_ALL
{
  int num_names = 0;

  // May also add the file name with a dot prepended, for swap file in same
  // dir as original file.
  if (prepend_dot) {
    names[num_names] = modname(path, ".sw?", true);
    if (names[num_names] == NULL) {
      return num_names;
    }
    num_names++;
  }

  // Form the normal swap file name pattern by appending ".sw?".
  names[num_names] = concat_fnames(path, ".sw?", false);
  if (num_names >= 1) {     // check if we have the same name twice
    char *p = names[num_names - 1];
    int i = (int)strlen(names[num_names - 1]) - (int)strlen(names[num_names]);
    if (i > 0) {
      p += i;               // file name has been expanded to full path
    }
    if (strcmp(p, names[num_names]) != 0) {
      num_names++;
    } else {
      xfree(names[num_names]);
    }
  } else {
    num_names++;
  }

  return num_names;
}


#if defined(HAVE_READLINK)

/// Resolve a symlink in the last component of a file name.
/// Note that f_resolve() does it for every part of the path, we don't do that
/// here.
///
/// @return  OK if it worked and the resolved link in "buf[MAXPATHL]",
///          FAIL otherwise
int resolve_symlink(const char *fname, char *buf)
{
  char tmp[MAXPATHL];
  int depth = 0;

  if (fname == NULL) {
    return FAIL;
  }

  // Put the result so far in tmp[], starting with the original name.
  xstrlcpy(tmp, fname, MAXPATHL);

  while (true) {
    // Limit symlink depth to 100, catch recursive loops.
    if (++depth == 100) {
      semsg(_("E773: Symlink loop for \"%s\""), fname);
      return FAIL;
    }

    int ret = (int)readlink(tmp, buf, MAXPATHL - 1);
    if (ret <= 0) {
      if (errno == EINVAL || errno == ENOENT) {
        // Found non-symlink or not existing file, stop here.
        // When at the first level use the unmodified name, skip the
        // call to vim_FullName().
        if (depth == 1) {
          return FAIL;
        }

        // Use the resolved name in tmp[].
        break;
      }

      // There must be some error reading links, use original name.
      return FAIL;
    }
    buf[ret] = NUL;

    // Check whether the symlink is relative or absolute.
    // If it's relative, build a new path based on the directory
    // portion of the filename (if any) and the path the symlink
    // points to.
    if (path_is_absolute(buf)) {
      STRCPY(tmp, buf);
    } else {
      char *tail = path_tail(tmp);
      if (strlen(tail) + strlen(buf) >= MAXPATHL) {
        return FAIL;
      }
      STRCPY(tail, buf);
    }
  }

  // Try to resolve the full name of the file so that the swapfile name will
  // be consistent even when opening a relative symlink from different
  // working directories.
  return vim_FullName(tmp, buf, MAXPATHL, true);
}
#endif

/// Make swap file name out of the file name and a directory name.
///
/// @return  pointer to allocated memory or NULL.
char *makeswapname(char *fname, char *ffname, buf_T *buf, char *dir_name)
{
  char *fname_res = fname;
#ifdef HAVE_READLINK
  char fname_buf[MAXPATHL];

  // Expand symlink in the file name, so that we put the swap file with the
  // actual file instead of with the symlink.
  if (resolve_symlink(fname, fname_buf) == OK) {
    fname_res = fname_buf;
  }
#endif
  int len = (int)strlen(dir_name);

  char *s = dir_name + len;
  if (after_pathsep(dir_name, s)
      && len > 1
      && s[-1] == s[-2]) {  // Ends with '//', Use Full path
    char *r = NULL;
    s = make_percent_swname(dir_name, fname_res);
    if (s != NULL) {
      r = modname(s, ".swp", false);
      xfree(s);
    }
    return r;
  }

  // Prepend a '.' to the swap file name for the current directory.
  char *r = modname(fname_res, ".swp",
                    dir_name[0] == '.' && dir_name[1] == NUL);
  if (r == NULL) {          // out of memory
    return NULL;
  }

  s = get_file_in_dir(r, dir_name);
  xfree(r);
  return s;
}

/// Get file name to use for swap file or backup file.
/// Use the name of the edited file "fname" and an entry in the 'dir' or 'bdir'
/// option "dname".
/// - If "dname" is ".", return "fname" (swap file in dir of file).
/// - If "dname" starts with "./", insert "dname" in "fname" (swap file
///   relative to dir of file).
/// - Otherwise, prepend "dname" to the tail of "fname" (swap file in specific
///   dir).
///
/// The return value is an allocated string and can be NULL.
///
/// @param dname  don't use "dirname", it is a global for Alpha
char *get_file_in_dir(char *fname, char *dname)
{
  char *retval;

  char *tail = path_tail(fname);

  if (dname[0] == '.' && dname[1] == NUL) {
    retval = xstrdup(fname);
  } else if (dname[0] == '.' && vim_ispathsep(dname[1])) {
    if (tail == fname) {            // no path before file name
      retval = concat_fnames(dname + 2, tail, true);
    } else {
      char save_char = *tail;
      *tail = NUL;
      char *t = concat_fnames(fname, dname + 2, true);
      *tail = save_char;
      retval = concat_fnames(t, tail, true);
      xfree(t);
    }
  } else {
    retval = concat_fnames(dname, tail, true);
  }

  return retval;
}

/// Print the ATTENTION message: info about an existing swap file.
///
/// @param buf  buffer being edited
/// @param fname  swap file name
static void attention_message(buf_T *buf, char *fname)
{
  assert(buf->b_fname != NULL);

  no_wait_return++;
  (void)emsg(_("E325: ATTENTION"));
  msg_puts(_("\nFound a swap file by the name \""));
  msg_home_replace(fname);
  msg_puts("\"\n");
  const time_t swap_mtime = swapfile_info(fname);
  msg_puts(_("While opening file \""));
  msg_outtrans(buf->b_fname);
  msg_puts("\"\n");
  FileInfo file_info;
  if (!os_fileinfo(buf->b_fname, &file_info)) {
    msg_puts(_("      CANNOT BE FOUND"));
  } else {
    msg_puts(_("             dated: "));
    time_t x = file_info.stat.st_mtim.tv_sec;
    char ctime_buf[50];
    msg_puts(os_ctime_r(&x, ctime_buf, sizeof(ctime_buf), true));
    if (swap_mtime != 0 && x > swap_mtime) {
      msg_puts(_("      NEWER than swap file!\n"));
    }
  }
  // Some of these messages are long to allow translation to
  // other languages.
  msg_puts(_("\n(1) Another program may be editing the same file.  If this is"
             " the case,\n    be careful not to end up with two different"
             " instances of the same\n    file when making changes."
             "  Quit, or continue with caution.\n"));
  msg_puts(_("(2) An edit session for this file crashed.\n"));
  msg_puts(_("    If this is the case, use \":recover\" or \"vim -r "));
  msg_outtrans(buf->b_fname);
  msg_puts(_("\"\n    to recover the changes (see \":help recovery\").\n"));
  msg_puts(_("    If you did this already, delete the swap file \""));
  msg_outtrans(fname);
  msg_puts(_("\"\n    to avoid this message.\n"));
  cmdline_row = msg_row;
  no_wait_return--;
}

/// Trigger the SwapExists autocommands.
///
/// @return  a value for equivalent to do_dialog() (see below):
///          0: still need to ask for a choice
///          1: open read-only
///          2: edit anyway
///          3: recover
///          4: delete it
///          5: quit
///          6: abort
static int do_swapexists(buf_T *buf, char *fname)
{
  set_vim_var_string(VV_SWAPNAME, fname, -1);
  set_vim_var_string(VV_SWAPCHOICE, NULL, -1);

  // Trigger SwapExists autocommands with <afile> set to the file being
  // edited.  Disallow changing directory here.
  allbuf_lock++;
  apply_autocmds(EVENT_SWAPEXISTS, buf->b_fname, NULL, false, NULL);
  allbuf_lock--;

  set_vim_var_string(VV_SWAPNAME, NULL, -1);

  switch (*get_vim_var_str(VV_SWAPCHOICE)) {
  case 'o':
    return 1;
  case 'e':
    return 2;
  case 'r':
    return 3;
  case 'd':
    return 4;
  case 'q':
    return 5;
  case 'a':
    return 6;
  }

  return 0;
}

/// Find out what name to use for the swap file for buffer 'buf'.
///
/// Several names are tried to find one that does not exist. Last directory in
/// option is automatically created.
///
/// @note If BASENAMELEN is not correct, you will get error messages for
///   not being able to open the swap or undo file.
/// @note May trigger SwapExists autocmd, pointers may change!
///
/// @param[in]  buf  Buffer for which swap file names needs to be found.
/// @param[in,out]  dirp  Pointer to a list of directories. When out of memory,
///                       is set to NULL. Is advanced to the next directory in
///                       the list otherwise.
/// @param[in]  old_fname  Allowed existing swap file name. Except for this
///                        case, name of the non-existing file is used.
/// @param[in,out]  found_existing_dir  If points to true, then new directory
///                                     for swap file is not created. At first
///                                     findswapname() call this argument must
///                                     point to false. This parameter may only
///                                     be set to true by this function, it is
///                                     never set to false.
///
/// @return [allocated] Name of the swap file.
char *findswapname(buf_T *buf, char **dirp, char *old_fname, bool *found_existing_dir)
  FUNC_ATTR_WARN_UNUSED_RESULT FUNC_ATTR_NONNULL_ARG(1, 2, 4)
{
  char *buf_fname = buf->b_fname;

  // Isolate a directory name from *dirp and put it in dir_name.
  // First allocate some memory to put the directory name in.
  const size_t dir_len = strlen(*dirp) + 1;
  char *dir_name = xmalloc(dir_len);
  (void)copy_option_part(dirp, dir_name, dir_len, ",");

  // we try different names until we find one that does not exist yet
  char *fname = makeswapname(buf_fname, buf->b_ffname, buf, dir_name);

  while (true) {
    size_t n;
    if (fname == NULL) {        // must be out of memory
      break;
    }
    if ((n = strlen(fname)) == 0) {        // safety check
      XFREE_CLEAR(fname);
      break;
    }
    // check if the swapfile already exists
    // Extra security check: When a swap file is a symbolic link, this
    // is most likely a symlink attack.
    FileInfo file_info;
    bool file_or_link_found = os_fileinfo_link(fname, &file_info);
    if (!file_or_link_found) {
      break;
    }

    // A file name equal to old_fname is OK to use.
    if (old_fname != NULL && path_fnamecmp(fname, old_fname) == 0) {
      break;
    }

    // get here when file already exists
    if (fname[n - 2] == 'w' && fname[n - 1] == 'p') {   // first try
      // If we get here the ".swp" file really exists.
      // Give an error message, unless recovering, no file name, we are
      // viewing a help file or when the path of the file is different
      // (happens when all .swp files are in one directory).
      if (!recoverymode && buf_fname != NULL
          && !buf->b_help && !(buf->b_flags & BF_DUMMY)) {
        bool differ = swapfile_fname_differs(fname, buf->b_ffname);

        // give the ATTENTION message when there is an old swap file
        // for the current file, and the buffer was not recovered.
        if (differ == false && !(curbuf->b_flags & BF_RECOVERED)
            && vim_strchr(p_shm, SHM_ATTENTION) == NULL) {
          int choice = 0;

          process_still_running = false;
          // It's safe to delete the swap file if all these are true:
          // - the edited file exists
          // - the swap file has no changes and looks OK
          if (os_path_exists(buf->b_fname) && swapfile_unchanged(fname)) {
            choice = 4;
            if (p_verbose > 0) {
              verb_msg(_("Found a swap file that is not useful, deleting it"));
            }
          }

          // If there is a SwapExists autocommand and we can handle the
          // response, trigger it.  It may return 0 to ask the user anyway.
          if (choice == 0
              && swap_exists_action != SEA_NONE
              && has_autocmd(EVENT_SWAPEXISTS, buf_fname, buf)) {
            choice = do_swapexists(buf, fname);
          }

          if (choice == 0) {
            // Show info about the existing swap file.
            attention_message(buf, fname);

            // We don't want a 'q' typed at the more-prompt
            // interrupt loading a file.
            got_int = false;

            // If vimrc has "simalt ~x" we don't want it to
            // interfere with the prompt here.
            flush_buffers(FLUSH_TYPEAHEAD);
          }

          if (swap_exists_action != SEA_NONE && choice == 0) {
            const char *const sw_msg_1 = _("Swap file \"");
            const char *const sw_msg_2 = _("\" already exists!");

            const size_t fname_len = strlen(fname);
            const size_t sw_msg_1_len = strlen(sw_msg_1);
            const size_t sw_msg_2_len = strlen(sw_msg_2);

            const size_t name_len = sw_msg_1_len + fname_len + sw_msg_2_len + 5;

            char *const name = xmalloc(name_len);
            memcpy(name, sw_msg_1, sw_msg_1_len + 1);
            home_replace(NULL, fname, &name[sw_msg_1_len], fname_len, true);
            xstrlcat(name, sw_msg_2, name_len);
            choice = do_dialog(VIM_WARNING, _("VIM - ATTENTION"),
                               name,
                               process_still_running
                               ? _("&Open Read-Only\n&Edit anyway\n&Recover"
                                   "\n&Quit\n&Abort") :
                               _("&Open Read-Only\n&Edit anyway\n&Recover"
                                 "\n&Delete it\n&Quit\n&Abort"),
                               1, NULL, false);

            if (process_still_running && choice >= 4) {
              choice++;                 // Skip missing "Delete it" button.
            }
            xfree(name);

            // pretend screen didn't scroll, need redraw anyway
            msg_reset_scroll();
          }

          if (choice > 0) {
            switch (choice) {
            case 1:
              buf->b_p_ro = true;
              break;
            case 2:
              break;
            case 3:
              swap_exists_action = SEA_RECOVER;
              break;
            case 4:
              os_remove(fname);
              break;
            case 5:
              swap_exists_action = SEA_QUIT;
              break;
            case 6:
              swap_exists_action = SEA_QUIT;
              got_int = true;
              break;
            }

            // If the file was deleted this fname can be used.
            if (!os_path_exists(fname)) {
              break;
            }
          } else {
            msg_puts("\n");
            if (msg_silent == 0) {
              // call wait_return() later
              need_wait_return = true;
            }
          }
        }
      }
    }

    // Change the ".swp" extension to find another file that can be used.
    // First decrement the last char: ".swo", ".swn", etc.
    // If that still isn't enough decrement the last but one char: ".svz"
    // Can happen when editing many "No Name" buffers.
    if (fname[n - 1] == 'a') {          // ".s?a"
      if (fname[n - 2] == 'a') {        // ".saa": tried enough, give up
        emsg(_("E326: Too many swap files found"));
        XFREE_CLEAR(fname);
        break;
      }
      fname[n - 2]--;                   // ".svz", ".suz", etc.
      fname[n - 1] = 'z' + 1;
    }
    fname[n - 1]--;                     // ".swo", ".swn", etc.
  }

  if (os_isdir(dir_name)) {
    *found_existing_dir = true;
  } else if (!*found_existing_dir && **dirp == NUL) {
    int ret;
    char *failed_dir;
    if ((ret = os_mkdir_recurse(dir_name, 0755, &failed_dir, NULL)) != 0) {
      semsg(_("E303: Unable to create directory \"%s\" for swap file, "
              "recovery impossible: %s"),
            failed_dir, os_strerror(ret));
      xfree(failed_dir);
    }
  }

  xfree(dir_name);
  return fname;
}

/// Compare current file name with file name from swap file.
/// Try to use inode numbers when possible.
/// Return non-zero when files are different.
///
/// When comparing file names a few things have to be taken into consideration:
/// - When working over a network the full path of a file depends on the host.
///   We check the inode number if possible.  It is not 100% reliable though,
///   because the device number cannot be used over a network.
/// - When a file does not exist yet (editing a new file) there is no inode
///   number.
/// - The file name in a swap file may not be valid on the current host.  The
///   "~user" form is used whenever possible to avoid this.
///
/// This is getting complicated, let's make a table:
///
///              ino_c  ino_s  fname_c  fname_s  differ =
///
/// both files exist -> compare inode numbers:
///              != 0   != 0     X        X      ino_c != ino_s
///
/// inode number(s) unknown, file names available -> compare file names
///              == 0    X       OK       OK     fname_c != fname_s
///               X     == 0     OK       OK     fname_c != fname_s
///
/// current file doesn't exist, file for swap file exist, file name(s) not
/// available -> probably different
///              == 0   != 0    FAIL      X      true
///              == 0   != 0     X       FAIL    true
///
/// current file exists, inode for swap unknown, file name(s) not
/// available -> probably different
///              != 0   == 0    FAIL      X      true
///              != 0   == 0     X       FAIL    true
///
/// current file doesn't exist, inode for swap unknown, one file name not
/// available -> probably different
///              == 0   == 0    FAIL      OK     true
///              == 0   == 0     OK      FAIL    true
///
/// current file doesn't exist, inode for swap unknown, both file names not
/// available -> compare file names
///              == 0   == 0    FAIL     FAIL    fname_c != fname_s
///
/// Only the last 32 bits of the inode will be used. This can't be changed
/// without making the block 0 incompatible with 32 bit versions.
///
/// @param fname_c  current file name
/// @param fname_s  file name from swap file
bool fnamecmp_ino(char *fname_c, char *fname_s, long ino_block0)
{
  uint64_t ino_c = 0;               // ino of current file
  uint64_t ino_s;                   // ino of file from swap file
  char buf_c[MAXPATHL];             // full path of fname_c
  char buf_s[MAXPATHL];             // full path of fname_s
  int retval_c;                     // flag: buf_c valid
  int retval_s;                     // flag: buf_s valid

  FileInfo file_info;
  if (os_fileinfo(fname_c, &file_info)) {
    ino_c = os_fileinfo_inode(&file_info);
  }

  // First we try to get the inode from the file name, because the inode in
  // the swap file may be outdated.  If that fails (e.g. this path is not
  // valid on this machine), use the inode from block 0.
  if (os_fileinfo(fname_s, &file_info)) {
    ino_s = os_fileinfo_inode(&file_info);
  } else {
    ino_s = (uint64_t)ino_block0;
  }

  if (ino_c && ino_s) {
    return ino_c != ino_s;
  }

  // One of the inode numbers is unknown, try a forced vim_FullName() and
  // compare the file names.
  retval_c = vim_FullName(fname_c, buf_c, MAXPATHL, true);
  retval_s = vim_FullName(fname_s, buf_s, MAXPATHL, true);
  if (retval_c == OK && retval_s == OK) {
    return strcmp(buf_c, buf_s) != 0;
  }

  // Can't compare inodes or file names, guess that the files are different,
  // unless both appear not to exist at all, then compare with the file name
  // in the swap file.
  if (ino_s == 0 && ino_c == 0 && retval_c == FAIL && retval_s == FAIL) {
    return strcmp(fname_c, fname_s) != 0;
  }
  return true;
}
