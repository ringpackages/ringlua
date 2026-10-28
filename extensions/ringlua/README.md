# RingLua Extension

## Introduction

This extension embeds the Lua 5.5 scripting language in Ring.

Lua URL: https://www.lua.org

The Lua source code is bundled in the `lua55` folder and compiled with the extension.

The functions and constants of this extension are loaded with the RingLua package, together with the high-level classes (`LuaState` and `LuaMultiState`) from `ring/libraries/lualib`:

```ring
load "lualib.ring"

oLua = new LuaState
pState = oLua.getPointer()
lua_pushinteger(pState, 42)
lua_setglobal(pState, "answer")
oLua.doString('print(answer)')
oLua.close()
```

## Files

| File | Description |
|------|-------------|
| `lua.cf` | Configuration file for the Ring code generator |
| `ring_lua.c` | Generated C source code |
| `ring_lua.rh` | Generated constants |
| `lua.ring` | Loads the extension library and the constants |
| `gencode.bat`, `gencode.sh` | Generate `ring_lua.c` and `ring_lua.rh` from `lua.cf` |
| `buildvc.bat`, `buildvc_x64.bat` | Build using Visual C/C++ (32-bit and 64-bit) |
| `buildgcc.sh` | Build using GCC (Linux) |
| `buildclang.sh` | Build using Clang (macOS) |
| `lua55` | Lua 5.5 source code |

## Functions

The functions take a `lua_State` pointer as the first parameter.

### State Management

`luaL_newstate()` returns a managed pointer: when no variable refers to it, the Ring VM closes the state (calls `lua_close()`). `lua_close()` closes the state now and sets the pointer to `NULL` (copies of the pointer become `NULL` too, and closing a `NULL` pointer does nothing). Don't pass a closed pointer to the other functions.

`luaL_newstate()`, `lua_close()`, `lua_newthread()`, `lua_closethread()`, `lua_version()`, `lua_status()`, `luaL_openselectedlibs()`

### Stack Manipulation

`lua_absindex()`, `lua_gettop()`, `lua_settop()`, `lua_pop()`, `lua_pushvalue()`, `lua_rotate()`, `lua_copy()`, `lua_checkstack()`, `lua_xmove()`

### Type Checking

`lua_type()`, `lua_typename()`, `lua_isnumber()`, `lua_isstring()`, `lua_isinteger()`, `lua_isfunction()`, `lua_iscfunction()`, `lua_istable()`, `lua_isboolean()`, `lua_isnil()`, `lua_isnone()`, `lua_isnoneornil()`, `lua_isuserdata()`, `lua_isyieldable()`

### Value Access

`lua_tonumber()`, `lua_tointeger()`, `lua_tonumberx()`, `lua_tointegerx()`, `lua_toboolean()`, `lua_tolstring()`, `luaL_tolstring()`, `luaL_checklstring()`, `lua_rawlen()`, `luaL_len()`, `lua_stringtonumber()`

### Push Values

`lua_pushnil()`, `lua_pushnumber()`, `lua_pushinteger()`, `lua_pushstring()`, `lua_pushlstring()`, `lua_pushboolean()`

### Tables

`lua_getglobal()`, `lua_gettable()`, `lua_getfield()`, `lua_geti()`, `lua_rawget()`, `lua_rawgeti()`, `lua_createtable()`, `lua_getmetatable()`, `lua_setglobal()`, `lua_settable()`, `lua_setfield()`, `lua_seti()`, `lua_rawset()`, `lua_rawseti()`, `lua_setmetatable()`, `lua_next()`, `luaL_getsubtable()`

### Comparison and Arithmetic

`lua_arith()`, `lua_rawequal()`, `lua_compare()`, `lua_concat()`, `lua_len()`

### Load and Call

`luaL_loadstring()`, `luaL_loadfilex()`, `lua_pcall()`, `lua_call()`, `luaL_dostring_errormsg()`, `luaL_dofile_errormsg()`

`lua_call()` is not protected: an error in the called function ends the program. Use `lua_pcall()` instead.

### Protected Access

`lua_getglobal()`, `lua_setglobal()`, `lua_getfield()`, `lua_setfield()`, `lua_gettable()`, `lua_settable()`, `lua_geti()`, `lua_seti()`, `lua_arith()`, `lua_compare()`, `lua_concat()`, `lua_len()`, `luaL_len()`, `luaL_tolstring()`, `luaL_checklstring()` and `lua_error()` can raise Lua errors (from metamethods or invalid values). Called directly from Ring, an error in them ends the program (Lua panic).

Use the protected versions below, or the `LuaState` class (which uses them):

| Function | Description |
|----------|-------------|
| `lua_pgetglobal(pState, cName)` | Push the global `cName`; returns `LUA_OK`, or an error code with the error message pushed |
| `lua_psetglobal(pState, cName)` | Pop the value on the top and set the global `cName`; returns `LUA_OK`, or an error code with the error message pushed |
| `lua_pgetfield(pState, nIndex, cKey)` | Push `t[cKey]` for the table at `nIndex`; returns `LUA_OK`, or an error code with the error message pushed |
| `lua_psetfield(pState, nIndex, cKey)` | Pop the value on the top and set `t[cKey]` for the table at `nIndex`; returns `LUA_OK`, or an error code with the error message pushed |

`luaL_dostring_errormsg(pState, cCode)` and `luaL_dofile_errormsg(pState, cFileName)` run Lua code and return the error message, or `""` on success. The stack is left as it was before the call (values returned by the chunk are removed).

### Auxiliary Library

`luaL_getmetafield()`, `luaL_callmeta()`, `luaL_newmetatable()`, `luaL_setmetatable()`, `luaL_where()`, `luaL_ref()`, `luaL_unref()`, `luaL_gsub()`, `luaL_traceback()`, `lua_error()`, `lua_toclose()`, `lua_closeslot()`

### Garbage Collection

`lua_gc_collect()`, `lua_gc_stop()`, `lua_gc_restart()`, `lua_gc_count()`, `lua_gc_step()`, `lua_gc_isrunning()`

### Data Exchange

| Function | Description |
|----------|-------------|
| `lua_toring_table(pState, nIndex)` | Return the Lua table at a stack index as a Ring list of `[key, value]` pairs (nested tables become nested lists; a table that contains itself becomes `"table"`) |
| `lua_pushring_table(pState, aList)` | Push a Ring list as a Lua table (`[key, value]` sub-lists become fields, other sub-lists become nested tables) |
| `lua_pushring_variable(pState, cName, cValue)` | Set a Lua global string (protected: errors are raised as Ring errors) |
| `lua_pushring_number(pState, cName, nValue)` | Set a Lua global number (whole numbers become Lua integers; protected) |
| `lua_callring_function(pState, cFunc, aArgs)` | Call a Lua global function with the items of `aArgs`; returns the stack size after the call, or `-1` with the error message pushed |

## Constants

`LUA_OK`, `LUA_YIELD`, `LUA_ERRRUN`, `LUA_ERRSYNTAX`, `LUA_ERRMEM`, `LUA_ERRERR`, `LUA_MULTRET`, `LUA_REGISTRYINDEX`, `LUA_RIDX_GLOBALS`, `LUA_RIDX_MAINTHREAD`, `LUA_MINSTACK`

Types: `LUA_TNONE`, `LUA_TNIL`, `LUA_TBOOLEAN`, `LUA_TLIGHTUSERDATA`, `LUA_TNUMBER`, `LUA_TSTRING`, `LUA_TTABLE`, `LUA_TFUNCTION`, `LUA_TUSERDATA`, `LUA_TTHREAD`, `LUA_NUMTYPES`

Operators: `LUA_OPADD`, `LUA_OPSUB`, `LUA_OPMUL`, `LUA_OPMOD`, `LUA_OPPOW`, `LUA_OPDIV`, `LUA_OPIDIV`, `LUA_OPBAND`, `LUA_OPBOR`, `LUA_OPBXOR`, `LUA_OPSHL`, `LUA_OPSHR`, `LUA_OPUNM`, `LUA_OPBNOT`, `LUA_OPEQ`, `LUA_OPLT`, `LUA_OPLE`

Garbage collector: `LUA_GCSTOP`, `LUA_GCRESTART`, `LUA_GCCOLLECT`, `LUA_GCCOUNT`, `LUA_GCCOUNTB`, `LUA_GCSTEP`, `LUA_GCISRUNNING`, `LUA_GCGEN`, `LUA_GCINC`, `LUA_GCPARAM`

Debug hooks: `LUA_HOOKCALL`, `LUA_HOOKRET`, `LUA_HOOKLINE`, `LUA_HOOKCOUNT`, `LUA_HOOKTAILCALL`, `LUA_MASKCALL`, `LUA_MASKRET`, `LUA_MASKLINE`, `LUA_MASKCOUNT`

## Building

| Platform | Commands |
|----------|----------|
| Windows (32-bit) | `gencode.bat` then `buildvc.bat` |
| Windows (64-bit) | `gencode.bat` then `buildvc_x64.bat` |
| Linux | `./gencode.sh` then `./buildgcc.sh` |
| macOS | `./gencode.sh` then `./buildclang.sh` |
