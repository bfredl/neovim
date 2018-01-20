functional/ui (7 or so failing tests)

functional/eval (1 failing test)
[  FAILED  ] test/functional/eval/server_spec.lua @ 28: serverstart(), serverstop() sets v:servername _on

^ caused not by us??


functional/normal
functional/legacy
-------------------
nvim: /home/tim/programming/repo/neovim/src/nvim/mark_extended.c:951: extmark_col_adjust_delete: Assertion `start_effected_range <= endcol' failed.

^ seems to be fixed by just removing the assert..
