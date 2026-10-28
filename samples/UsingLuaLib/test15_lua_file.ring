/*
	LuaLib Demo 15 - Run Lua files
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 15: Run Lua files
# -------------------------------------------------------
? "Demo 15: Run Lua files..."

cFileName = "test15_script.lua"

write(cFileName, '
	function square(x)
		return x * x
	end
	loaded_message = "Script loaded from file"
')

oLua = new LuaState

oLua.doFile(cFileName)
? oLua.getString("loaded_message")
aResult = oLua.callFunction("square", [12])
? "square(12) = " + aResult[1]

# loadFile() compiles the file and leaves the chunk on the stack
? "loadFile() status : " + oLua.loadFile(cFileName)
? "Top of the stack  : " + oLua.typeAt(-1)
oLua.pop(1)

? "doFileSafe() on a missing file: " + oLua.doFileSafe("missing_file.lua")

oLua.close()
remove(cFileName)
? ""
