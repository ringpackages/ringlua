# Open all the standard Lua libraries (used with luaL_openselectedlibs)
LUALIB_ALLLIBS = -1

# Mode used when loading Lua files (text and binary chunks)
LUALIB_LOADMODE = "bt"

# Whole numbers up to 2^53 are pushed to Lua as integers
LUALIB_MAXEXACTINT = 9007199254740992

# Extra stack slots reserved before pushing values
LUALIB_STACKSLOTS = 8

# Prefix of the errors raised by the LuaState class
LUALIB_ERRORPREFIX = "Lua Error: "

# Lua global used by LuaState.eval() to return the captured output
LUALIB_EVALRESULT = "__ringlua_eval_result"
