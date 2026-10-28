/*
	LuaLib Demo 12 - String processing
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 12: String processing
# -------------------------------------------------------
? "Demo 12: String processing..."

oLua = new LuaState

oLua.setString("input_text", "The Ring Programming Language is Great")
oLua.doString('
	upper_text = string.upper(input_text)
	lower_text = string.lower(input_text)
	reversed = string.reverse(input_text)
	word_count = 0
	for word in input_text:gmatch("%S+") do
		word_count = word_count + 1
	end
')

? "Original : " + oLua.getString("input_text")
? "Upper    : " + oLua.getString("upper_text")
? "Lower    : " + oLua.getString("lower_text")
? "Reversed : " + oLua.getString("reversed")
? "Words    : " + oLua.getNumber("word_count")

oLua.close()
? ""
