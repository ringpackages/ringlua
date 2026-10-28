/*
	LuaLib Demo 1 - Execute Lua code
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 1: Execute Lua code
# -------------------------------------------------------
? "Demo 1: Execute Lua code..."

oLua = new LuaState

oLua.doString('
	print("Hello from Lua 5.5!")
	print("Lua version: " .. _VERSION)
')

oLua.close()
? ""
