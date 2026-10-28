/*
	LuaLib Demo 5 - Exchange tables and lists
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 5: Exchange tables and lists
# -------------------------------------------------------
? "Demo 5: Exchange tables and lists..."

oLua = new LuaState

# Send a Ring list to Lua as a sequence table
aColors = ["Red", "Green", "Blue", "Yellow"]
oLua.setTable("colors", aColors)

oLua.doString('
	print("Colors from Ring:")
	for i, color in ipairs(colors) do
		print("  " .. i .. ". " .. color)
	end
')

# Send [key, value] pairs
aConfig = [
	["host", "localhost"],
	["port", 8080],
	["debug", "true"]
]
oLua.setTable("config", aConfig)

oLua.doString('
	print("Config from Ring:")
	print("  host  = " .. config.host)
	print("  port  = " .. config.port)
	print("  debug = " .. config.debug)
')

# Get a Lua table back as a Ring list of [key, value] pairs
oLua.doString('
	person = {
		name = "Sample User",
		age = 30,
		city = "City A"
	}
')

aPerson = oLua.getTable("person")
? "Person table from Lua:"
nCount = len(aPerson)
for nItem = 1 to nCount
	? "  " + aPerson[nItem][1] + " = " + aPerson[nItem][2]
next

oLua.close()
? ""
