/*
	LuaLib Demo 9 - Garbage collection and version
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 9: Garbage collection and version
# -------------------------------------------------------
? "Demo 9: Garbage collection and version..."

oLua = new LuaState

? "Lua version : " + oLua.version()
? "GC running  : " + oLua.gcIsRunning()

oLua.doString('
	data = {}
	for i = 1, 100000 do data[i] = "item " .. i end
')
? "Memory (KB) : " + oLua.gcCount()

oLua.doString('data = nil')
oLua.gcCollect()
? "After collect, Memory (KB) : " + oLua.gcCount()

oLua.gcStop()
? "GC running after gcStop()    : " + oLua.gcIsRunning()
oLua.gcRestart()
? "GC running after gcRestart() : " + oLua.gcIsRunning()

oLua.close()
? ""
