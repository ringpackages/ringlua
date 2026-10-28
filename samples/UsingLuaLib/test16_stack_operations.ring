/*
	LuaLib Demo 16 - Stack operations
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 16: Stack operations
# -------------------------------------------------------
? "Demo 16: Stack operations..."

oLua = new LuaState

oLua.push("text")
oLua.push(25)
oLua.push(2.5)
oLua.push([1, 2, 3])
oLua.pushNil()

? "Stack size: " + oLua.stackSize()
nCount = oLua.stackSize()
for nIndex = 1 to nCount
	? "  [" + nIndex + "] " + oLua.typeAt(nIndex)
next

? "Value at 1: " + oLua.getAt(1)
? "Value at 2: " + oLua.getAt(2)
? "Value at 3: " + oLua.getAt(3)

oLua.pop(nCount)
? "Stack size after pop: " + oLua.stackSize()

? oLua.traceback("Traceback demo")

oLua.close()
? ""
