/*
	LuaLib Demo 6 - Error handling
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 6: Error handling
# -------------------------------------------------------
? "Demo 6: Error handling..."

oLua = new LuaState

# Safe methods return the error message instead of raising an error
cError = oLua.doStringSafe('error("This is a test error!")')
? "doStringSafe() error: " + cError

aResult = oLua.callFunctionSafe("nonexistent_function", [])
? "callFunctionSafe() success = " + aResult[1]
? "callFunctionSafe() error   = " + aResult[2]

# Errors inside a called function do not crash the program
oLua.doString('function divide(a, b) if b == 0 then error("division by zero") end return a / b end')
aResult = oLua.callFunctionSafe("divide", [10, 0])
? "divide(10, 0) success = " + aResult[1]
? "divide(10, 0) error   = " + aResult[2]

# The other methods raise a Ring error
try
	oLua.doString('local x = nil + 1')
catch
	? "Caught: " + cCatchError
done

oLua.close()
? ""
