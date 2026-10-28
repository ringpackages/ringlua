/*
	LuaLib Demo 2 - Pass variables from Ring to Lua
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 2: Pass variables from Ring to Lua
# -------------------------------------------------------
? "Demo 2: Pass variables from Ring to Lua..."

oLua = new LuaState

oLua.setString("name", "Ring Programming Language")
oLua.setNumber("year", 2016)
oLua.setBoolean("isAwesome", True)
oLua.setValue("tags", ["fast", "small", "flexible"])

oLua.doString('
	print("Language: " .. name)
	print("Year: " .. year)
	if isAwesome then
		print(name .. " is awesome!")
	end
	print("Tags: " .. table.concat(tags, ", "))
')

oLua.close()
? ""
