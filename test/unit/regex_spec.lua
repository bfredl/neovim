local helpers = require("test.unit.helpers")(after_each)

local cimport = helpers.cimport
local ffi = helpers.ffi

local regexp = cimport("./src/nvim/regexp.h")
local search = cimport("./src/nvim/search.h")

local function regexp_multi()
  return regexp.vim_regsub_multi()
end

local function search_regcomp(pat)
  local regmatch = ffi.new("regmmatch_T *")
  return search.search_regcomp(, search.RE_SUBST, search.SEARCH_HIS, regmatch)
end

describe("sublen is what we need #blah", function()
  local rv = search_regcomp()
end)
