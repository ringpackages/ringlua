func luaRun cCode
	oLua = new LuaState
	cError = oLua.doStringSafe(cCode)
	oLua.close()
	if cError
		raise(LUALIB_ERRORPREFIX + cError)
	ok
	return True

func luaRunFile cFileName
	oLua = new LuaState
	cError = oLua.doFileSafe(cFileName)
	oLua.close()
	if cError
		raise(LUALIB_ERRORPREFIX + cError)
	ok
	return True

func luaEval cCode
	oLua = new LuaState
	cOutput = oLua.eval(cCode)
	oLua.close()
	return cOutput

func luaPushValue pLuaState, vValue
	if ! lua_checkstack(pLuaState, LUALIB_STACKSLOTS)
		raise(LUALIB_ERRORPREFIX + "Stack overflow")
	ok
	if isString(vValue)
		lua_pushlstring(pLuaState, vValue, len(vValue))
	elseif isNumber(vValue)
		if fabs(vValue) <= LUALIB_MAXEXACTINT and vValue = floor(vValue)
			lua_pushinteger(pLuaState, vValue)
		else
			lua_pushnumber(pLuaState, vValue)
		ok
	elseif isList(vValue)
		lua_pushring_table(pLuaState, vValue)
	else
		lua_pushnil(pLuaState)
	ok

func luaStackValue pLuaState, nIndex
	switch lua_type(pLuaState, nIndex)
		on LUA_TSTRING
			return lua_tolstring(pLuaState, nIndex)
		on LUA_TNUMBER
			return lua_tonumber(pLuaState, nIndex)
		on LUA_TBOOLEAN
			return lua_toboolean(pLuaState, nIndex)
		on LUA_TTABLE
			return lua_toring_table(pLuaState, nIndex)
	off
	return NULL

func luaPopValue pLuaState
	vValue = luaStackValue(pLuaState, -1)
	lua_pop(pLuaState, 1)
	return vValue
