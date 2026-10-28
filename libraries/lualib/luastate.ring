class LuaState

	# Created here (not in init) so both "new LuaState" and "new LuaState()" work
	# luaL_newstate() returns a managed pointer: the Ring VM closes the state
	# when no variable refers to it (copies of the object share the pointer)
	pState = luaL_newstate()
	luaL_openselectedlibs(pState, LUALIB_ALLLIBS, 0)

	func init
		return self

	func close
		lua_close(pState)

	func isOpen
		return ! isNULL(pState)

	func getPointer
		checkOpen()
		return pState

	func version
		return lua_version(getPointer())

	# Code Execution

	func doString cCode
		cError = doStringSafe(cCode)
		if cError
			raise(LUALIB_ERRORPREFIX + cError)
		ok
		return True

	func doStringSafe cCode
		return luaL_dostring_errormsg(getPointer(), cCode)

	func doFile cFileName
		cError = doFileSafe(cFileName)
		if cError
			raise(LUALIB_ERRORPREFIX + cError)
		ok
		return True

	func doFileSafe cFileName
		return luaL_dofile_errormsg(getPointer(), cFileName)

	func loadString cCode
		return luaL_loadstring(getPointer(), cCode)

	func loadFile cFileName
		return luaL_loadfilex(getPointer(), cFileName, LUALIB_LOADMODE)

	func eval cCode
		cCapture = '
			local __output = {}
			local __oldprint = print
			print = function(...)
				local args = {...}
				local parts = {}
				for i = 1, select("#", ...) do
					parts[#parts + 1] = tostring(args[i])
				end
				__output[#__output + 1] = table.concat(parts, "\t")
			end
			local __ok, __err = pcall(function()
				' + cCode + '
			end)
			print = __oldprint
			if __ok then
				' + LUALIB_EVALRESULT + ' = table.concat(__output, "\n")
			else
				' + LUALIB_EVALRESULT + ' = "Error: " .. tostring(__err)
			end
		'
		cError = doStringSafe(cCapture)
		if cError
			return "Error: " + cError
		ok
		cOutput = getString(LUALIB_EVALRESULT)
		setNil(LUALIB_EVALRESULT)
		return cOutput

	# Global Variables

	func setValue cName, vValue
		checkOpen()
		luaPushValue(pState, vValue)
		setGlobal(cName)

	func setString cName, cValue
		setValue(cName, "" + cValue)

	func setNumber cName, nValue
		setValue(cName, 0 + nValue)

	func setBoolean cName, lValue
		checkOpen()
		checkStack()
		lua_pushboolean(pState, lValue)
		setGlobal(cName)

	func setNil cName
		checkOpen()
		luaPushValue(pState, NULL)
		setGlobal(cName)

	func setTable cName, aList
		if ! isList(aList) aList = [] ok
		setValue(cName, aList)

	func getValue cName
		checkOpen()
		pushGlobal(cName)
		return luaPopValue(pState)

	func getString cName
		checkOpen()
		pushGlobal(cName)
		cResult = ""
		nType = lua_type(pState, -1)
		if nType = LUA_TSTRING
			cResult = lua_tolstring(pState, -1)
		elseif nType = LUA_TNUMBER
			cResult = luaL_tolstring(pState, -1)
		ok
		lua_pop(pState, 1)
		return cResult

	func getNumber cName
		checkOpen()
		pushGlobal(cName)
		nResult = lua_tonumber(pState, -1)
		lua_pop(pState, 1)
		return nResult

	func getBoolean cName
		checkOpen()
		pushGlobal(cName)
		lResult = lua_toboolean(pState, -1)
		lua_pop(pState, 1)
		return lResult

	func getTable cName
		checkOpen()
		pushGlobal(cName)
		aResult = []
		if lua_istable(pState, -1)
			aResult = lua_toring_table(pState, -1)
		ok
		lua_pop(pState, 1)
		return aResult

	func getType cName
		checkOpen()
		pushGlobal(cName)
		cType = lua_typename(pState, lua_type(pState, -1))
		lua_pop(pState, 1)
		return cType

	# Function Calling

	func callFunction cFunc, aArgs
		checkOpen()
		if ! isList(aArgs) aArgs = [] ok
		nArgs = len(aArgs)
		if ! lua_checkstack(pState, nArgs + LUALIB_STACKSLOTS)
			raise(LUALIB_ERRORPREFIX + "Stack overflow")
		ok
		nTop = lua_gettop(pState)
		pushGlobal(cFunc)
		if ! lua_isfunction(pState, -1)
			lua_settop(pState, nTop)
			raise(LUALIB_ERRORPREFIX + "'" + cFunc + "' is not a function")
		ok
		for nArg = 1 to nArgs
			luaPushValue(pState, aArgs[nArg])
		next
		if lua_pcall(pState, nArgs, LUA_MULTRET, 0) != LUA_OK
			cError = errorMessage()
			lua_settop(pState, nTop)
			raise(LUALIB_ERRORPREFIX + cError)
		ok
		nNewTop = lua_gettop(pState)
		aResults = []
		for nIndex = nTop + 1 to nNewTop
			aResults + luaStackValue(pState, nIndex)
		next
		lua_settop(pState, nTop)
		return aResults

	func callFunctionSafe cFunc, aArgs
		try
			return [True, callFunction(cFunc, aArgs)]
		catch
			return [False, cCatchError]
		done

	# Table Fields

	func createTable cName
		checkOpen()
		checkStack()
		lua_createtable(pState, 0, 0)
		setGlobal(cName)

	func setTableField cTableName, cFieldName, vValue
		checkOpen()
		pushGlobal(cTableName)
		if ! lua_istable(pState, -1)
			lua_pop(pState, 1)
			raise(LUALIB_ERRORPREFIX + "'" + cTableName + "' is not a table")
		ok
		luaPushValue(pState, vValue)
		if lua_psetfield(pState, -2, cFieldName) != LUA_OK
			cError = errorMessage()
			lua_pop(pState, 2)
			raise(LUALIB_ERRORPREFIX + cError)
		ok
		lua_pop(pState, 1)

	func getTableField cTableName, cFieldName
		checkOpen()
		pushGlobal(cTableName)
		if ! lua_istable(pState, -1)
			lua_pop(pState, 1)
			return NULL
		ok
		if lua_pgetfield(pState, -1, cFieldName) != LUA_OK
			cError = errorMessage()
			lua_pop(pState, 2)
			raise(LUALIB_ERRORPREFIX + cError)
		ok
		vValue = luaPopValue(pState)
		lua_pop(pState, 1)
		return vValue

	# Garbage Collection

	func gcCollect
		return lua_gc_collect(getPointer())

	func gcStop
		return lua_gc_stop(getPointer())

	func gcRestart
		return lua_gc_restart(getPointer())

	func gcCount
		return lua_gc_count(getPointer())

	func gcStep nStepSize
		return lua_gc_step(getPointer(), nStepSize)

	func gcIsRunning
		return lua_gc_isrunning(getPointer())

	# Stack (Advanced)

	func stackSize
		return lua_gettop(getPointer())

	func pop nCount
		checkOpen()
		if nCount > lua_gettop(pState)
			nCount = lua_gettop(pState)
		ok
		lua_pop(pState, nCount)

	func push vValue
		luaPushValue(getPointer(), vValue)

	func pushNil
		luaPushValue(getPointer(), NULL)

	func typeAt nIndex
		checkOpen()
		return lua_typename(pState, lua_type(pState, nIndex))

	func getAt nIndex
		return luaStackValue(getPointer(), nIndex)

	func traceback cMsg
		checkOpen()
		if ! isString(cMsg) cMsg = "" ok
		checkStack()
		luaL_traceback(pState, pState, cMsg, 1)
		cResult = lua_tolstring(pState, -1)
		lua_pop(pState, 1)
		return cResult

	private

	func checkOpen
		if isNULL(pState)
			raise(LUALIB_ERRORPREFIX + "The Lua state is closed")
		ok

	func checkStack
		if ! lua_checkstack(pState, LUALIB_STACKSLOTS)
			raise(LUALIB_ERRORPREFIX + "Stack overflow")
		ok

	# Push the global cName; raises a Ring error if a metamethod fails
	func pushGlobal cName
		if lua_pgetglobal(pState, cName) != LUA_OK
			cError = errorMessage()
			lua_pop(pState, 1)
			raise(LUALIB_ERRORPREFIX + cError)
		ok

	# Pop the value on the top and set it as the global cName
	func setGlobal cName
		if lua_psetglobal(pState, cName) != LUA_OK
			cError = errorMessage()
			lua_pop(pState, 1)
			raise(LUALIB_ERRORPREFIX + cError)
		ok

	func errorMessage
		if lua_type(pState, -1) = LUA_TSTRING
			return lua_tolstring(pState, -1)
		ok
		return "Unknown Lua error"
