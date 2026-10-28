/*
	LuaLib Demo 4 - Call Lua functions
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 4: Call Lua functions
# -------------------------------------------------------
? "Demo 4: Call Lua functions..."

oLua = new LuaState

oLua.doString('
	function add(a, b)
		return a + b
	end

	function greet(name, age)
		return "Hello, " .. name .. "! You are " .. age .. " years old."
	end

	function multiReturn(x)
		return x * 2, x * 3, x * 4
	end

	function factorial(n)
		if n <= 1 then return 1 end
		return n * factorial(n - 1)
	end
')

aResult = oLua.callFunction("add", [10, 20])
? "add(10, 20) = " + aResult[1]

aResult = oLua.callFunction("greet", ["User 1", 30])
? "greet() = " + aResult[1]

aResult = oLua.callFunction("multiReturn", [5])
? "multiReturn(5) = " + aResult[1] + ", " + aResult[2] + ", " + aResult[3]

aResult = oLua.callFunction("factorial", [10])
? "factorial(10) = " + aResult[1]

oLua.close()
? ""
