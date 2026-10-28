/*
	LuaLib Demo 8 - Table fields
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 8: Table fields
# -------------------------------------------------------
? "Demo 8: Table fields..."

oLua = new LuaState

oLua.doString('myApp = { version = "1.0", author = "Unknown" }')
oLua.setTableField("myApp", "author", "Sample User")
oLua.setTableField("myApp", "language", "Ring")
oLua.setTableField("myApp", "build", 100)

? "myApp.author   = " + oLua.getTableField("myApp", "author")
? "myApp.version  = " + oLua.getTableField("myApp", "version")
? "myApp.language = " + oLua.getTableField("myApp", "language")
? "myApp.build    = " + oLua.getTableField("myApp", "build")

oLua.createTable("settings")
oLua.setTableField("settings", "theme", "dark")
oLua.doString('print("settings.theme = " .. settings.theme)')

oLua.close()
? ""
