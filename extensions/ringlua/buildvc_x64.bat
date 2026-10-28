cls
setlocal enableextensions enabledelayedexpansion
call ../../language/build/locatevc.bat auto x64
cl /c %ringcflags% ring_lua.c lua55/lapi.c lua55/lauxlib.c lua55/lbaselib.c lua55/lcode.c lua55/lcorolib.c lua55/lctype.c lua55/ldblib.c lua55/ldebug.c lua55/ldo.c lua55/ldump.c lua55/lfunc.c lua55/lgc.c lua55/linit.c lua55/liolib.c lua55/llex.c lua55/lmathlib.c lua55/lmem.c lua55/loadlib.c lua55/lobject.c lua55/lopcodes.c lua55/loslib.c lua55/lparser.c lua55/lstate.c lua55/lstring.c lua55/lstrlib.c lua55/ltable.c lua55/ltablib.c lua55/ltm.c lua55/lundump.c lua55/lutf8lib.c lua55/lvm.c lua55/lzio.c -I"lua55" -I"..\..\language\include"
link %ringldflags% ring_lua.obj lapi.obj lauxlib.obj lbaselib.obj lcode.obj lcorolib.obj lctype.obj ldblib.obj ldebug.obj ldo.obj ldump.obj lfunc.obj lgc.obj linit.obj liolib.obj llex.obj lmathlib.obj lmem.obj loadlib.obj lobject.obj lopcodes.obj loslib.obj lparser.obj lstate.obj lstring.obj lstrlib.obj ltable.obj ltablib.obj ltm.obj lundump.obj lutf8lib.obj lvm.obj lzio.obj ..\..\lib\ring.lib /DLL /OUT:..\..\bin\ring_lua.dll 
del *.obj
endlocal
