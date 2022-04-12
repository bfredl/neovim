
more core functionality has been exposed to lua. This includes the ability to define user commands, autocommands and mappings via the API and with lua callbacks as handlers. Color schemes can be defined as pure lua scripts, as `nvim_set_hl` now supports global highlights.

As for UI features, signs can now be defined via the `extmark` API. This enables a consistent inteface for a plugin to provide buffer highlights, virtual text and signs via one single mechanism. Global statusline can be used to replace the smaller window statusline with a single line spanning the entire screen.


### BREAKING CHANGES

* Support for Python 2 is dropped. For python 3, the minimum supported version is 3.6. Legacy `:pythonx` etc commands are still available, and always uses the python 3 provider.

* **api:** Existing usages of `nvim_buf_set_text` that use negative line numbers will be off-by-one.
* **highlight:** signature of `vim.highlight.range was changed`.

* **input:** distinguish between some input keys which previously were synonyms. This will break some exiting mappings.

`<cr>`, `<tab>` and `<esc>` are no longer considered equivalent to `<c-m>`, `<c-i>` and `<c-[`> respectively. In case your terminal or GUI supports distinguishing these keys, you can now map them separately. But even if your terminal only can send one code you might still need to change what variant is used in your config.

### Features

Core APIs:
* **api:** add support for lua function & description in keymap ([b411f43](https://github.com/neovim/neovim/commit/b411f436d3e2e8a902dbf879d00fc5ed0fc436d3))
* **api:** add api and lua autocmds ([991e472](https://github.com/neovim/neovim/commit/991e472881bf29805982b402c1a010cde051ded3))
* **api:** nvim_clear_autocmd ([b80651e](https://github.com/neovim/neovim/commit/b80651eda9c50d4e438f02af9311b18c5c202656))
* **api:** pass args table to autocommand callbacks ([30bc02c](https://github.com/neovim/neovim/commit/30bc02c6364f384e437a6f53b057522d585492fc))
* **api:** remove Lua autocommand callbacks when they return true ([#17784](https://github.com/neovim/neovim/issues/17784)) ([be35d3c](https://github.com/neovim/neovim/commit/be35d3c5ad501abb029279f8e1812d0e4525284f))
* **api:** implement nvim_{add,del}_user_command ([eff11b3](https://github.com/neovim/neovim/commit/eff11b3c3fcb9aa777deafb0a33b1523aa05b603))
* **api:** implement nvim_buf_get_text ([#15181](https://github.com/neovim/neovim/issues/15181)) ([11f7aee](https://github.com/neovim/neovim/commit/11f7aeed7aa83d342d19897d9a69ba9f32ece7f7))
* **api:** add nvim_get_option_value ([71ac00c](https://github.com/neovim/neovim/commit/71ac00ccb523383411b907b5fdf00a376e24a6f0))

* **highlight:** ns=0 to set :highlight namespace ([4aa0cdd](https://github.com/neovim/neovim/commit/4aa0cdd3aa117e032325edeb755107acd4ecbf84))
* **highlight:** support for blend in nvim_set_hl ([#17516](https://github.com/neovim/neovim/issues/17516)) ([b5bf487](https://github.com/neovim/neovim/commit/b5bf4877c0239767c1095e4567e67c222bea38a0))
* **api:** add strikethrough, nocombine to set_hl ([cb18545](https://github.com/neovim/neovim/commit/cb18545253259af339957316ab8361fb0cca48e5))
* **api:** relax statusline fillchar width check ([3011794](https://github.com/neovim/neovim/commit/3011794c8600f529bc049983a64ca99ae03908df))

lua:
* **lua:** add vim.keymap ([6d41f65](https://github.com/neovim/neovim/commit/6d41f65aa45f10a93ad476db01413abaac21f27d))
* **lua:** add vim.spell ([#16620](https://github.com/neovim/neovim/issues/16620)) ([e11a44a](https://github.com/neovim/neovim/commit/e11a44aa224ae59670b992a73bfb029f77a75e76))
* **lua:** add proper support of luv threads ([b87867e](https://github.com/neovim/neovim/commit/b87867e69e94d9784468a126f21c721446f080de))
* **lua:** make :lua =expr print result of expr ([d442546](https://github.com/neovim/neovim/commit/d44254641ffb5c9f185db4082d2bf1f04bf1117e))
* **lua:** handle lazy submodules in `:lua vim.` wildmenu completion ([5ed6080](https://github.com/neovim/neovim/commit/5ed60804fe69e97a699ca64422f4f7f4cc20f3da))
* **lua:** add notify_once() ([#16956](https://github.com/neovim/neovim/issues/16956)) ([d78e466](https://github.com/neovim/neovim/commit/d78e46679d2ff31916091f9368367ccc1539c299))
* **lua:** add support for multiple optional types in vim.validate ([#16864](https://github.com/neovim/neovim/issues/16864)) ([55c4393](https://github.com/neovim/neovim/commit/55c4393e9f80ac3e7233da889efce4f760e41664))
* **lua:** show proper verbose output for lua configuration ([ebfe083](https://github.com/neovim/neovim/commit/ebfe083337701534887ac3ea3d8e7ad47f7a206a))
* **lua:** more conversions between LuaRef and Vim Funcref ([c8656e4](https://github.com/neovim/neovim/commit/c8656e44d85502a1733df839b3cb3e8f239c5505))
* **lua:** support converting nested Funcref back to LuaRef ([#17749](https://github.com/neovim/neovim/issues/17749)) ([cac90d2](https://github.com/neovim/neovim/commit/cac90d2de728181edce7ba38fb9ad588d231651b))
* call __tostring on lua errors if possible before reporting to user ([81bffbd](https://github.com/neovim/neovim/commit/81bffbd147cd24580ac92fa9d9d85121151ca01f))

* filetype.lua ([#16600](https://github.com/neovim/neovim/issues/16600)) ([3fd454b](https://github.com/neovim/neovim/commit/3fd454bd4a6ceb1989d15cf2d3d5e11d7a253b2d))
* **filetype.lua:** add support for files under .git ([7a574e5](https://github.com/neovim/neovim/commit/7a574e54f2309eb9d267282619f9383413b85d08))
* **filetype.lua:** add support for patch files ([27b664a](https://github.com/neovim/neovim/commit/27b664a2de08301ca847c3b06a34df2be71e0caf))
* **filetype.lua:** add support for tmux.conf files ([94d5358](https://github.com/neovim/neovim/commit/94d53589221567444bac2cf6a3692906259fe4c6))
* **filetype.lua:** add support for txt files ([#16926](https://github.com/neovim/neovim/issues/16926)) ([a45b578](https://github.com/neovim/neovim/commit/a45b578dbe6ba02ba9a052a7b058f4243d38a07b))
* **filetype.lua:** Add typescript extension to filetype detection ([#16923](https://github.com/neovim/neovim/issues/16923)) ([8ade800](https://github.com/neovim/neovim/commit/8ade8009ee1fb508bf94ca6c8c3cd288f051c55b))
* **filetype.lua:** fix .cc file not detected ([c38d602](https://github.com/neovim/neovim/commit/c38d602b888a95a4b3b7a3b4241ce5b3e434eb35))
* **filetype.lua:** fix .env file not detected ([19864bd](https://github.com/neovim/neovim/commit/19864bd99529334909922e8d2a61a600fea7b29a))
* **filetype:** convert patterns for mail buffers ([#17238](https://github.com/neovim/neovim/issues/17238)) ([4458413](https://github.com/neovim/neovim/commit/4458413bc02a1308bd722611227664033916d6f7))
* **filetype:** support scripts.vim with filetype.lua ([#17517](https://github.com/neovim/neovim/issues/17517)) ([fdea157](https://github.com/neovim/neovim/commit/fdea15723fab6a3ee96218f13669d9f2e0a6d6d7))

UI and decorations:
* **decorations:** support signs ([30e4cc3](https://github.com/neovim/neovim/commit/30e4cc3b3f2133e9a7170da9da8175832681f39a))
* **extmarks:** add strict option ([11142f6](https://github.com/neovim/neovim/commit/11142f6ffe46da1f20c570333a2c05b6e3015f56))
* **api:** expose extmark more details ([5971b86](https://github.com/neovim/neovim/commit/5971b863383160d9bf744a9789c1fe5ca62b55a4))
* **api:** expose extmark right_gravity and end_right_gravity ([3d9ae9d](https://github.com/neovim/neovim/commit/3d9ae9d2dad88a4e2c2263dc7e256657842244c0))
* use nvim_buf_set_extmark for vim.highlight ([#16963](https://github.com/neovim/neovim/issues/16963)) ([b455e01](https://github.com/neovim/neovim/commit/b455e0179b4288c69e6231bfcf8d1c132b78f2fc))
* **statusline:** support multibyte fillchar ([be15ac0](https://github.com/neovim/neovim/commit/be15ac06badbea6b11390ad7d9c2ddd4aea73480))
* add support for global statusline ([5ab1229](https://github.com/neovim/neovim/commit/5ab122917474b3f9e88be4ee88bc6d627980cfe0)), closes [#9342](https://github.com/neovim/neovim/issues/9342)

Treesitter:

* **ui:** allow conceal to be defined in decorations and tree-sitter queries ([6eca9b6](https://github.com/neovim/neovim/commit/6eca9b69c4a1f40f27a6b41961af787327259de8))
* **tree-sitter:** allow Atom-style capture fallbacks ([#14196](https://github.com/neovim/neovim/issues/14196)) ([8ab5ec4](https://github.com/neovim/neovim/commit/8ab5ec4aaaeed27b1d8086d395171a52568378c2))
* **treesitter:** add more default groups to highlight map ([#17835](https://github.com/neovim/neovim/issues/17835)) ([6d648f5](https://github.com/neovim/neovim/commit/6d648f5594d580766fb28e45d797a4019d8b8149))
* **treesitter:** multiline match predicates ([6e6c36c](https://github.com/neovim/neovim/commit/6e6c36ca5bc31de39504a2949da85043d1469db8))
* **treesitter:** set allocator when possible ([b1e0aa6](https://github.com/neovim/neovim/commit/b1e0aa60f9a0c17084de07871d507576869b9559))
* **ts:** add support for multiline nodes in get_node_text ([#14999](https://github.com/neovim/neovim/issues/14999)) ([1f3c059](https://github.com/neovim/neovim/commit/1f3c0593eb1d4e54ce1edf35da67d184807a9280))
* **ts:** expose minimum language version to lua ([#17186](https://github.com/neovim/neovim/issues/17186)) ([8c140be](https://github.com/neovim/neovim/commit/8c140be31f0d203b63e7052e698fdfe253e0b5d4))
* **runtime:** add query filetype ([#17905](https://github.com/neovim/neovim/issues/17905)) ([2e85af4](https://github.com/neovim/neovim/commit/2e85af47d2584372f968b760cab3eeee65273424))

LSP and diagnostic:

* **diagnostic:** add "code" to the diagnostic structure ([#17510](https://github.com/neovim/neovim/issues/17510)) ([5d6006f](https://github.com/neovim/neovim/commit/5d6006f9bfc2f1f064adbcfa974da6976e867450))
* **diagnostic:** allow retrieving current diagnostic config ([c915571](https://github.com/neovim/neovim/commit/c915571b99d7e1ea99e29b103ca2ad37b5974027))
* **lsp,diagnostic:** open folds in jump-related functions ([#16520](https://github.com/neovim/neovim/issues/16520)) ([222ef0c](https://github.com/neovim/neovim/commit/222ef0c00d97aa2d5e17ca6b14aea037155595ee))
* **lsp:** add buf_detach_client ([#16250](https://github.com/neovim/neovim/issues/16250)) ([1b04da5](https://github.com/neovim/neovim/commit/1b04da52b3ce611e06b7d1c87af4a71c37ad127a))
* **lsp:** add handler for workspace/workspaceFolders ([#17149](https://github.com/neovim/neovim/issues/17149)) ([8e702c1](https://github.com/neovim/neovim/commit/8e702c14ac5fc481bc4a3c709e75e3c165326128))
* **lsp:** dynamically generate list title in response_to_list ([#17081](https://github.com/neovim/neovim/issues/17081)) ([574a582](https://github.com/neovim/neovim/commit/574a5822023939d534d922eaa345bb7e0633d2b8))
* **lsp:** enable default debounce of 150 ms ([#16908](https://github.com/neovim/neovim/issues/16908)) ([55a59e5](https://github.com/neovim/neovim/commit/55a59e56eda98f17448a1c318a346ae12d30fc05))
* **lsp:** skip or reduce debounce after idle ([#16881](https://github.com/neovim/neovim/issues/16881)) ([b680392](https://github.com/neovim/neovim/commit/b680392687eeaee521b19d79a1e7effdc2dc1ed7))
* **lsp:** use `vim.ui.select` for selecting lsp client ([#16531](https://github.com/neovim/neovim/issues/16531)) ([f99f3d9](https://github.com/neovim/neovim/commit/f99f3d90523c2dfd5cdbf45bc1d626b5cd64e9c0))

Initial work to support remote TUI (and ui client library):

* **ui_client:** connect to remote ui  ([a4400bf](https://github.com/neovim/neovim/commit/a4400bf8cda8ace4c4aab67bc73a1820478f46f1))
* **ui_client:** implement event handlers ([794d274](https://github.com/neovim/neovim/commit/794d2744f33562326172801ddd729853e7135347))
* **ui_client:** handle resize events ([c6640d0](https://github.com/neovim/neovim/commit/c6640d0d700f977913606277418be546404d5fd7))
* **ui_client:** implement async paste handling ([55b6ade](https://github.com/neovim/neovim/commit/55b6ade7fee36283dc2853494edf9a5ac2dd4be9))
* **ui_client:** pass user input to remote server ([6636160](https://github.com/neovim/neovim/commit/663616033834c5da3b8f48b0bd0db783fc92db31))

* **--headless:** add on_print callback to stdioopen ([a4069a3](https://github.com/neovim/neovim/commit/a4069a3eed65f14b1149c6cda8638dcb49ab5027))
* add autocommand event when search wraps around ([#8487](https://github.com/neovim/neovim/issues/8487)) ([8ad6015](https://github.com/neovim/neovim/commit/8ad60154099678b23b78bc8142a168753f53648c))
* add vim.tbl_get ([#17831](https://github.com/neovim/neovim/issues/17831)) ([69f1de8](https://github.com/neovim/neovim/commit/69f1de86dca28d6e339351082df1309ef4fbb6a6))
* **autocmd:** add Recording autocmds ([8a4e26c](https://github.com/neovim/neovim/commit/8a4e26c6fe7530a0e24268cd373f0d4e53fe81e1))
* **autocmd:** populate v:event in RecordingLeave ([#16828](https://github.com/neovim/neovim/issues/16828)) ([f65b0d4](https://github.com/neovim/neovim/commit/f65b0d4236eef69b02390a51cf335b0836f35801))
* **completion:** support selecting item via API from Lua mapping ([c7aa646](https://github.com/neovim/neovim/commit/c7aa64631d721d140741206167d9a6ce766f1153))
* **eval/method:** partially port v8.1.1993 ([4efcb72](https://github.com/neovim/neovim/commit/4efcb72bb758ce93e86fa3ef520e009d01d4891b)), closes [#10848](https://github.com/neovim/neovim/issues/10848)
* **eval/method:** partially port v8.1.1996 ([2ee0bc0](https://github.com/neovim/neovim/commit/2ee0bc09d9becd71ca864b4d754b63b152d1ce5b))
* **eval/method:** partially port v8.1.2004 ([0f4510c](https://github.com/neovim/neovim/commit/0f4510cb1a48c4c4d7b23a45f57d087329d4364d))
* **eval:** partially port v8.2.0878 ([d746f5a](https://github.com/neovim/neovim/commit/d746f5aa418f86828aef689a2c4f8d5b53c9f7de)), closes [vim/vim#5481](https://github.com/vim/vim/issues/5481)
* **eval:** port emsg from v8.2.3284 ([8adbba7](https://github.com/neovim/neovim/commit/8adbba7ac38d7a0b4e1f602f6522b9403c11fc7e))
* **events:** add DirChangedPre ([059d36e](https://github.com/neovim/neovim/commit/059d36e326e31fc9bc6055d7c999f86d94fa9bd5)), closes [vim/vim#9721](https://github.com/vim/vim/issues/9721)
* **events:** support SIGWINCH for Signal event [#18029](https://github.com/neovim/neovim/issues/18029) ([b2cb05b](https://github.com/neovim/neovim/commit/b2cb05b53e61d162044f71227e0ffeacbf59a4bb)), closes [#15411](https://github.com/neovim/neovim/issues/15411)
* **hardcopy:** check gui colours for highlights first ([e5b5cbd](https://github.com/neovim/neovim/commit/e5b5cbd19c6374540ee6ffa6d8b27ceb8a293f65))
* **highlight:** support color names for cterm ([dc24eeb](https://github.com/neovim/neovim/commit/dc24eeb9febaa331e660e14c3c325fd0977b6b93))
* ignore nore on <Plug> maps ([0347875](https://github.com/neovim/neovim/commit/0347875a5c11258ebb6377a1ab79b04fe9c55bc9))
* **input:** delay some conversions to vgetc() ([d7488bf](https://github.com/neovim/neovim/commit/d7488bf38677b5d6b1df3a88e45b3d2f21527eb4))
* **input:** enable <tab>/<c-i>, <cr>/<c-m>, <esc>/<c-[> pairs unconditionally ([ed88ca7](https://github.com/neovim/neovim/commit/ed88ca75034a48916d165e88459c791c450df550))
* **keymap:** add F38-F63 keys ([#17893](https://github.com/neovim/neovim/issues/17893)) ([9da0023](https://github.com/neovim/neovim/commit/9da0023a666e83e6b9f777871553177473bfa9ce))
* **keymap:** return nil from an expr keymap ([58140a9](https://github.com/neovim/neovim/commit/58140a94283b1c6e45099c89e66a0c94e9d90931))
* **mappings:** considering map description when filtering ([#17423](https://github.com/neovim/neovim/issues/17423)) ([9a74c2b](https://github.com/neovim/neovim/commit/9a74c2b04ac8f54a17925a437b5a2f03b18f6281))
* **provider:** remove support for python2 and python3.[3-5] ([baec0d3](https://github.com/neovim/neovim/commit/baec0d3152afeab3007ebb505f3fc274511db434))
* **remote:** add basic --remote support ([5862176](https://github.com/neovim/neovim/commit/5862176764c7a86d5fdd2685122810e14a3d5b02))
* **runtime:** import cleanadd.vim from Vim ([#17699](https://github.com/neovim/neovim/issues/17699)) ([d33aebb](https://github.com/neovim/neovim/commit/d33aebb821b7e7c9197b035c9152859e0b6ed712))
* **runtime:** include Lua in C++ ftplugin ([#17843](https://github.com/neovim/neovim/issues/17843)) ([02fd00c](https://github.com/neovim/neovim/commit/02fd00c042d2b8a66c892dd31c1659ee98a1dbbf))
* **runtime:** new checkhealth filetype ([#16660](https://github.com/neovim/neovim/issues/16660)) ([734fba0](https://github.com/neovim/neovim/commit/734fba0d88cc9ff3b5fa24328e5ba7852e0e3211))
* **term:** use vterm_output_set_callback() ([7813b48](https://github.com/neovim/neovim/commit/7813b48645bf2af11c2d18f4e4154a74d4dad662))
* **test:** use nvim_exec in helpers.source() [#16064](https://github.com/neovim/neovim/issues/16064) ([72652cb](https://github.com/neovim/neovim/commit/72652cbc46f568128bfc296ba63fb2d26941da8e)), closes [#16071](https://github.com/neovim/neovim/issues/16071)
* trigger ModeChanged for terminal modes ([fdfd1ed](https://github.com/neovim/neovim/commit/fdfd1eda434b70b02b4cb804546c97ef8ff09049))
* **tui:** add error logging ([#16615](https://github.com/neovim/neovim/issues/16615)) ([34d88ed](https://github.com/neovim/neovim/commit/34d88edaec33bc75a60618fd62e570aa235c03ea))
* **tui:** add support for `CSI 4 : [2,4,5] m` ([f89fb41](https://github.com/neovim/neovim/commit/f89fb41a7a8b499159bfa44afa26dd17a845af45)), closes [#17362](https://github.com/neovim/neovim/issues/17362)
* **tui:** enable CSI u keys ([a11ff55](https://github.com/neovim/neovim/commit/a11ff555557ada858d74d8192badb725d77fdbb0))
* **vim-patch.sh:** support additional args for -s ([0ec92bb](https://github.com/neovim/neovim/commit/0ec92bb4634ef19798eef065fdef3d6afb43ccc5))

