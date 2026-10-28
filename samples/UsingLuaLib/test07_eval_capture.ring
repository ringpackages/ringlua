/*
	LuaLib Demo 7 - Capture print() output
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 7: Capture print() output
# -------------------------------------------------------
? "Demo 7: Capture print() output..."

oLua = new LuaState

cOutput = oLua.eval('
	for i = 1, 5 do
		print("Count: " .. i)
	end
	print("Done counting!")
')

? "Captured output:"
? cOutput

oLua.close()
? ""
