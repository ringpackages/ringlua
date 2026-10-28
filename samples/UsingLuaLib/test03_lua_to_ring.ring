/*
	LuaLib Demo 3 - Get variables from Lua
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 3: Get variables from Lua
# -------------------------------------------------------
? "Demo 3: Get variables from Lua..."

oLua = new LuaState

oLua.doString('
	greeting = "Hello from Lua!"
	pi_value = 3.14159265358979
	count = 42
	flag = true
')

? "greeting = " + oLua.getString("greeting")
? "pi_value = " + oLua.getNumber("pi_value")
? "count    = " + oLua.getNumber("count")
? "flag     = " + oLua.getBoolean("flag")
? "type of greeting: " + oLua.getType("greeting")
? "type of pi_value: " + oLua.getType("pi_value")
? "getValue(count) : " + oLua.getValue("count")

oLua.close()
? ""
