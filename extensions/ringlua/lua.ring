if isWindows()
	LoadLib("ring_lua.dll")
but ismacosx()
	LoadLib("libring_lua.dylib")
else
	LoadLib("libring_lua.so")
ok

load "ring_lua.rh"
