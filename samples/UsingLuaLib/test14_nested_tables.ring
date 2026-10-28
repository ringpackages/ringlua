/*
	LuaLib Demo 14 - Nested tables
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 14: Nested tables
# -------------------------------------------------------
? "Demo 14: Nested tables..."

oLua = new LuaState

# Nested Ring lists become nested Lua tables
oLua.setTable("server", [
	["name", "Server A"],
	["ports", [80, 443, 8080]],
	["limits", [ ["users", 100], ["files", 5000] ]]
])

oLua.doString('
	print("name   = " .. server.name)
	print("ports  = " .. table.concat(server.ports, ", "))
	print("users  = " .. server.limits.users)
	print("files  = " .. server.limits.files)
	server.limits.users = server.limits.users * 2
')

# Nested Lua tables come back as nested Ring lists
aLimits = oLua.getTableField("server", "limits")
? "Limits from Lua:"
nCount = len(aLimits)
for nItem = 1 to nCount
	? "  " + aLimits[nItem][1] + " = " + aLimits[nItem][2]
next

oLua.close()
? ""
