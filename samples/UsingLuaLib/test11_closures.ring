/*
	LuaLib Demo 11 - Closures
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 11: Closures
# -------------------------------------------------------
? "Demo 11: Closures..."

oLua = new LuaState

oLua.doString('
	-- Fibonacci generator using a closure
	function fibonacci()
		local a, b = 0, 1
		return function()
			a, b = b, a + b
			return a
		end
	end

	-- Generate the first 10 Fibonacci numbers
	fib_result = {}
	local gen = fibonacci()
	for i = 1, 10 do
		fib_result[i] = gen()
	end
')

aFib = oLua.getTable("fib_result")
cFib = "Fibonacci: "
nCount = len(aFib)
for nItem = 1 to nCount
	cFib += "" + aFib[nItem][2] + " "
next
? cFib

oLua.close()
? ""
