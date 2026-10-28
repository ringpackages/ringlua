/*
	LuaLib Demo 10 - Multiple Lua states
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 10: Multiple Lua states
# -------------------------------------------------------
? "Demo 10: Multiple Lua states..."

oMulti = new LuaMultiState

nState1 = oMulti.newState()
nState2 = oMulti.newState()

oMulti.state(nState1).setNumber("x", 100)
oMulti.state(nState2).setNumber("x", 200)

oMulti.state(nState1).doString('result = x * 2')
oMulti.state(nState2).doString('result = x * 3')

? "State 1 result: " + oMulti.state(nState1).getNumber("result")
? "State 2 result: " + oMulti.state(nState2).getNumber("result")

# Each state has its own globals
oMulti.doStringAll('counter = (counter or 0) + 1')
oMulti.state(nState1).doString('counter = counter + 10')
? "States count    : " + oMulti.count()
? "State 1 counter : " + oMulti.state(nState1).getNumber("counter")
? "State 2 counter : " + oMulti.state(nState2).getNumber("counter")

oMulti.closeAll()
? ""
