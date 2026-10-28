/*
	LuaLib Demo 13 - Math
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 13: Math
# -------------------------------------------------------
? "Demo 13: Math..."

oLua = new LuaState

oLua.doString('
	math_results = {
		{ "pi", math.pi },
		{ "sqrt2", math.sqrt(2) },
		{ "sin90", math.sin(math.rad(90)) },
		{ "log10_100", math.log(100, 10) },
		{ "max_val", math.max(10, 20, 5, 30, 15) }
	}
')

# Each item is [index, [[1, name], [2, value]]]
aMath = oLua.getTable("math_results")
? "Math results from Lua:"
nCount = len(aMath)
for nItem = 1 to nCount
	aEntry = aMath[nItem][2]
	? "  " + aEntry[1][2] + " = " + aEntry[2][2]
next

oLua.close()
? ""
