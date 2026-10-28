/*
	LuaLib Demo 17 - Quick functions
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 17: Quick functions
# -------------------------------------------------------
? "Demo 17: Quick functions..."

# Each function creates a Lua state, runs the code and closes the state

luaRun('print("luaRun(): Hello from Lua!")')

cOutput = luaEval('
	for i = 1, 3 do
		print("luaEval(): line " .. i)
	end
')
? cOutput

try
	luaRun('error("something went wrong")')
catch
	? "Caught: " + cCatchError
done
? ""
