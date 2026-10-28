aPackageInfo = [
	:name = "The RingLua Package",
	:description = "Our RingLua package (Embedding Lua 5.5) using the Ring programming language",
	:folder = "ringlua",
	:developer = "Mahmoud Fayed",
	:email = "msfclipper@yahoo.com",
	:license = "MIT License",
	:version = "1.0.0",
	:ringversion = "1.27",
	:versions = 	[
		[
			:version = "1.0.0",
			:branch = "master"
		]
	],
	:libs = 	[

	],
	:files = 	[
		"main.ring"
	],
	:ringfolderfiles = 	[
		"bin/load/lualib.ring",
		"extensions/ringlua/README.md",
		"extensions/ringlua/buildclang.sh",
		"extensions/ringlua/buildgcc.sh",
		"extensions/ringlua/buildvc.bat",
		"extensions/ringlua/buildvc_x64.bat",
		"extensions/ringlua/gencode.bat",
		"extensions/ringlua/gencode.sh",
		"extensions/ringlua/lua.cf",
		"extensions/ringlua/lua.ring",
		"extensions/ringlua/lua55/Makefile",
		"extensions/ringlua/lua55/lapi.c",
		"extensions/ringlua/lua55/lapi.h",
		"extensions/ringlua/lua55/lauxlib.c",
		"extensions/ringlua/lua55/lauxlib.h",
		"extensions/ringlua/lua55/lbaselib.c",
		"extensions/ringlua/lua55/lcode.c",
		"extensions/ringlua/lua55/lcode.h",
		"extensions/ringlua/lua55/lcorolib.c",
		"extensions/ringlua/lua55/lctype.c",
		"extensions/ringlua/lua55/lctype.h",
		"extensions/ringlua/lua55/ldblib.c",
		"extensions/ringlua/lua55/ldebug.c",
		"extensions/ringlua/lua55/ldebug.h",
		"extensions/ringlua/lua55/ldo.c",
		"extensions/ringlua/lua55/ldo.h",
		"extensions/ringlua/lua55/ldump.c",
		"extensions/ringlua/lua55/lfunc.c",
		"extensions/ringlua/lua55/lfunc.h",
		"extensions/ringlua/lua55/lgc.c",
		"extensions/ringlua/lua55/lgc.h",
		"extensions/ringlua/lua55/linit.c",
		"extensions/ringlua/lua55/liolib.c",
		"extensions/ringlua/lua55/ljumptab.h",
		"extensions/ringlua/lua55/llex.c",
		"extensions/ringlua/lua55/llex.h",
		"extensions/ringlua/lua55/llimits.h",
		"extensions/ringlua/lua55/lmathlib.c",
		"extensions/ringlua/lua55/lmem.c",
		"extensions/ringlua/lua55/lmem.h",
		"extensions/ringlua/lua55/loadlib.c",
		"extensions/ringlua/lua55/lobject.c",
		"extensions/ringlua/lua55/lobject.h",
		"extensions/ringlua/lua55/lopcodes.c",
		"extensions/ringlua/lua55/lopcodes.h",
		"extensions/ringlua/lua55/lopnames.h",
		"extensions/ringlua/lua55/loslib.c",
		"extensions/ringlua/lua55/lparser.c",
		"extensions/ringlua/lua55/lparser.h",
		"extensions/ringlua/lua55/lprefix.h",
		"extensions/ringlua/lua55/lstate.c",
		"extensions/ringlua/lua55/lstate.h",
		"extensions/ringlua/lua55/lstring.c",
		"extensions/ringlua/lua55/lstring.h",
		"extensions/ringlua/lua55/lstrlib.c",
		"extensions/ringlua/lua55/ltable.c",
		"extensions/ringlua/lua55/ltable.h",
		"extensions/ringlua/lua55/ltablib.c",
		"extensions/ringlua/lua55/ltm.c",
		"extensions/ringlua/lua55/ltm.h",
		"extensions/ringlua/lua55/lua.c",
		"extensions/ringlua/lua55/lua.h",
		"extensions/ringlua/lua55/lua.hpp",
		"extensions/ringlua/lua55/luac.c",
		"extensions/ringlua/lua55/luaconf.h",
		"extensions/ringlua/lua55/lualib.h",
		"extensions/ringlua/lua55/lundump.c",
		"extensions/ringlua/lua55/lundump.h",
		"extensions/ringlua/lua55/lutf8lib.c",
		"extensions/ringlua/lua55/lvm.c",
		"extensions/ringlua/lua55/lvm.h",
		"extensions/ringlua/lua55/lzio.c",
		"extensions/ringlua/lua55/lzio.h",
		"extensions/ringlua/ring_lua.c",
		"extensions/ringlua/ring_lua.rh",
		"libraries/lualib/functions.ring",
		"libraries/lualib/globals.ring",
		"libraries/lualib/lualib.ring",
		"libraries/lualib/luamultistate.ring",
		"libraries/lualib/luastate.ring",
		"samples/UsingLuaLib/test01_execute_code.ring",
		"samples/UsingLuaLib/test02_ring_to_lua.ring",
		"samples/UsingLuaLib/test03_lua_to_ring.ring",
		"samples/UsingLuaLib/test04_call_functions.ring",
		"samples/UsingLuaLib/test05_tables_lists.ring",
		"samples/UsingLuaLib/test06_error_handling.ring",
		"samples/UsingLuaLib/test07_eval_capture.ring",
		"samples/UsingLuaLib/test08_table_fields.ring",
		"samples/UsingLuaLib/test09_garbage_collection.ring",
		"samples/UsingLuaLib/test10_multiple_states.ring",
		"samples/UsingLuaLib/test11_closures.ring",
		"samples/UsingLuaLib/test12_string_processing.ring",
		"samples/UsingLuaLib/test13_math.ring",
		"samples/UsingLuaLib/test14_nested_tables.ring",
		"samples/UsingLuaLib/test15_lua_file.ring",
		"samples/UsingLuaLib/test16_stack_operations.ring",
		"samples/UsingLuaLib/test17_quick_functions.ring",
		"samples/UsingLuaLib/test18_data_processing.ring",
		"samples/UsingLuaLib/test19_game_scripting.ring",
		"tools/ring2exe/libs/lua.ring"
	],
	:windowsfiles = 	[

	],
	:linuxfiles = 	[

	],
	:ubuntufiles = 	[

	],
	:fedorafiles = 	[

	],
	:freebsdfiles = 	[

	],
	:macosfiles = 	[

	],
	:windowsringfolderfiles = 	[
		"bin/ring_lua.dll"
	],
	:linuxringfolderfiles = 	[

	],
	:ubunturingfolderfiles = 	[

	],
	:fedoraringfolderfiles = 	[

	],
	:freebsdringfolderfiles = 	[

	],
	:macosringfolderfiles = 	[

	],
	:run = "ring main.ring",
	:windowsrun = "",
	:linuxrun = "",
	:macosrun = "",
	:ubunturun = "",
	:fedorarun = "",
	:setup = "",
	:windowssetup = "",
	:linuxsetup = "",
	:macossetup = "",
	:ubuntusetup = "",
	:fedorasetup = "",
	:remove = "",
	:windowsremove = "",
	:linuxremove = "",
	:macosremove = "",
	:ubunturemove = "",
	:fedoraremove = ""
]