class LuaMultiState

	aStates = []

	func newState
		aStates + new LuaState
		return len(aStates)

	func state nIndex
		if nIndex < 1 or nIndex > len(aStates)
			raise(LUALIB_ERRORPREFIX + "Invalid Lua state index")
		ok
		return ref(aStates[nIndex])

	func closeState nIndex
		if nIndex >= 1 and nIndex <= len(aStates)
			aStates[nIndex].close()
		ok

	func closeAll
		nCount = len(aStates)
		for nIndex = 1 to nCount
			aStates[nIndex].close()
		next
		aStates = []

	func count
		return len(aStates)

	func doStringAll cCode
		aResults = []
		nCount = len(aStates)
		for nIndex = 1 to nCount
			if aStates[nIndex].isOpen()
				aResults + aStates[nIndex].doStringSafe(cCode)
			else
				aResults + "Lua state is closed"
			ok
		next
		return aResults
