# RingLua Package

## Overview

RingLua package for the Ring programming language

RingLua embeds the Lua 5.5 scripting engine in Ring programs. Use it to run Lua code and files, pass data between Ring and Lua, call Lua functions with arguments, and use the Lua standard library from Ring.

The package has two layers:

1. **C Extension** (`ring/extensions/ringlua`): `lua.cf` is processed by the Ring code generator to produce `ring_lua.c`, which wraps the Lua C API. The Lua 5.5 source code is bundled in the `lua55` folder and compiled with the extension, so there are no external dependencies.
2. **Ring Library** (`ring/libraries/lualib`): the `LuaState` and `LuaMultiState` classes and the quick functions.

## Features

- Lua 5.5 bundled and compiled into the extension
- Run Lua strings and files (`doString()`, `doFile()`)
- Pass strings, numbers, booleans and lists between Ring and Lua
- Nested Ring lists become nested Lua tables and back
- Whole Ring numbers are passed as Lua integers (Lua prints `30`, not `30.0`)
- Call Lua functions with arguments and get all the return values
- Errors raised as Ring errors, or returned as strings by the `Safe` methods
- Capture the output of Lua `print()` with `eval()`
- Read and write fields of Lua tables
- Control the Lua garbage collector
- Run several isolated Lua states with `LuaMultiState`
- Direct access to the Lua stack and the Lua C API for advanced usage

## Installation

	ringpm install ringlua from ringpackages

## Quick Start

```ring
load "lualib.ring"

oLua = new LuaState

# Execute Lua code
oLua.doString('print("Hello from Lua!")')

# Pass data from Ring to Lua
oLua.setString("name", "Ring")
oLua.setNumber("version", 1.28)

# Get data from Lua
oLua.doString('result = name .. " v" .. version')
? oLua.getString("result")            # Output: Ring v1.28

# Call Lua functions
oLua.doString('function add(a, b) return a + b end')
aResult = oLua.callFunction("add", [10, 20])
? aResult[1]                           # Output: 30

# Exchange tables and lists
oLua.setTable("colors", ["Red", "Green", "Blue"])
oLua.doString('table.insert(colors, "Yellow")')
? oLua.getTable("colors")

oLua.close()
```

### Quick Functions

```ring
load "lualib.ring"

luaRun('print("Hello from Lua!")')
luaRunFile("script.lua")
? luaEval('for i = 1, 3 do print(i) end')
```

---

## API Reference

### Quick Functions

`luaRun()`, `luaRunFile()` and `luaEval()` create a Lua state, run the code, then close the state.

| Function | Description |
|----------|-------------|
| `luaRun(cCode)` | Run a Lua code string; raises a Ring error if the code fails |
| `luaRunFile(cFileName)` | Run a Lua file; raises a Ring error if the file fails |
| `luaEval(cCode)` | Run a Lua code string and return the output of `print()` as a string |
| `luaPushValue(pLuaState, vValue)` | Push a Ring string, number or list onto the stack of a `lua_State` pointer (other values push `nil`); strings may contain `char(0)` |
| `luaStackValue(pLuaState, nIndex)` | Return the value at a stack index as a Ring value, without popping it |
| `luaPopValue(pLuaState)` | Return the value on the top of the stack and pop it |

---

### LuaState Class

A Lua state with all the standard Lua libraries loaded. Both `new LuaState` and `new LuaState()` create the state.

The Lua state is a managed C pointer: when no variable refers to it, the Ring VM closes it and frees its memory (when it frees its temporary lists: at the latest when the function returns, regularly inside loops, or at program exit). Call `close()` to free the state at once.

Errors raised by Lua code, by metamethods (for example a strict `_G` that rejects unknown globals) and by invalid calls are raised as Ring errors starting with `Lua Error: `, and can be caught with `try/catch`. They never end the program.

Copies of a `LuaState` object (for example `oCopy = oLua`, or `oState = oMulti.state(1)`) share the same Lua state, which stays open while any copy exists. After `close()` on any copy, the other copies raise `Lua Error: The Lua state is closed` instead of using the freed state.

#### Lifecycle

| Method | Description |
|--------|-------------|
| `close()` | Close the Lua state and free its resources now (calling it again does nothing) |
| `isOpen()` | Return true if the state is open |
| `version()` | Return the Lua version number (`505` for Lua 5.5) |
| `getPointer()` | Return the `lua_State` pointer, to use with the Lua C API functions (raises an error if the state is closed) |

#### Code Execution

| Method | Description |
|--------|-------------|
| `doString(cCode)` | Run a Lua code string; raises a Ring error on failure |
| `doStringSafe(cCode)` | Run a Lua code string; returns the error message, or `""` on success |
| `doFile(cFileName)` | Run a Lua file; raises a Ring error on failure |
| `doFileSafe(cFileName)` | Run a Lua file; returns the error message, or `""` on success |
| `loadString(cCode)` | Compile a Lua code string and push it as a function; returns `LUA_OK` on success |
| `loadFile(cFileName)` | Compile a Lua file and push it as a function; returns `LUA_OK` on success |
| `eval(cCode)` | Run a Lua code string and return the output of `print()` (lines joined with new lines); errors are returned as `"Error: ..."` |

#### Global Variables

| Method | Description |
|--------|-------------|
| `setValue(cName, vValue)` | Set a Lua global to a Ring string, number or list |
| `setString(cName, cValue)` | Set a Lua global string |
| `setNumber(cName, nValue)` | Set a Lua global number |
| `setBoolean(cName, lValue)` | Set a Lua global boolean |
| `setNil(cName)` | Set a Lua global to `nil` |
| `setTable(cName, aList)` | Set a Lua global table from a Ring list (see below) |
| `getValue(cName)` | Return a Lua global as a Ring value (string, number, list for tables, `NULL` for nil and other types) |
| `getString(cName)` | Return a Lua global string (numbers are converted; other types return `""`) |
| `getNumber(cName)` | Return a Lua global number (`0` if it is not a number) |
| `getBoolean(cName)` | Return a Lua global as a boolean (`1` or `0`) |
| `getTable(cName)` | Return a Lua global table as a Ring list (`[]` if it is not a table) |
| `getType(cName)` | Return the Lua type name of a global: `"nil"`, `"boolean"`, `"number"`, `"string"`, `"table"`, `"function"`, `"userdata"` or `"thread"` |

#### Function Calling

| Method | Description |
|--------|-------------|
| `callFunction(cFunc, aArgs)` | Call a Lua global function and return a list of its return values; raises a Ring error on failure |
| `callFunctionSafe(cFunc, aArgs)` | Same as `callFunction()`; returns `[True, aResults]` on success or `[False, cError]` on failure |

#### Table Fields

| Method | Description |
|--------|-------------|
| `createTable(cName)` | Create an empty Lua global table |
| `setTableField(cTableName, cFieldName, vValue)` | Set a field of a Lua global table; raises a Ring error if the global is not a table |
| `getTableField(cTableName, cFieldName)` | Return a field of a Lua global table (`NULL` if the global is not a table) |

#### Garbage Collection

| Method | Description |
|--------|-------------|
| `gcCollect()` | Run a full garbage collection cycle |
| `gcStop()` | Stop the garbage collector |
| `gcRestart()` | Restart the garbage collector |
| `gcCount()` | Return the memory used by Lua in KB |
| `gcStep(nStepSize)` | Run an incremental garbage collection step |
| `gcIsRunning()` | Return true if the garbage collector is running |

#### Stack (Advanced)

| Method | Description |
|--------|-------------|
| `stackSize()` | Return the number of values on the Lua stack |
| `pop(nCount)` | Pop values from the stack |
| `push(vValue)` | Push a Ring string, number or list |
| `pushNil()` | Push `nil` |
| `typeAt(nIndex)` | Return the Lua type name of the value at a stack index |
| `getAt(nIndex)` | Return the value at a stack index as a Ring value |
| `traceback(cMsg)` | Return a Lua stack traceback that starts with `cMsg` |

#### Lists and Tables

Ring lists are converted to Lua tables as follows:

- Strings and numbers become sequence items: `["a", "b"]` becomes `{"a", "b"}`
- A sub-list of two items whose first item is a string or a number is a key/value pair: `[["host", "localhost"]]` becomes `{host = "localhost"}`
- Any other sub-list becomes a nested table: `[["ports", [80, 443]]]` becomes `{ports = {80, 443}}`

Lua tables are converted to Ring lists of `[key, value]` pairs (nested tables become nested lists):

```ring
oLua.doString('person = { name = "Sample User", tags = {"a", "b"} }')
? oLua.getTable("person")
# [ ["name", "Sample User"], ["tags", [ [1, "a"], [2, "b"] ]] ]
```

Lua booleans become `1` or `0`; functions, userdata and threads become their type name (for example `"function"`). A table that contains itself becomes the string `"table"`, and nesting is limited to 64 levels. A Ring list that contains itself (using `ref()`) becomes `nil` at the repeated place. Keys follow the order of Lua's `pairs()`, so the order of keys that are not sequence indexes is not guaranteed.

---

### LuaMultiState Class

A pool of isolated Lua states.

| Method | Description |
|--------|-------------|
| `newState()` | Create a new `LuaState`, add it to the pool and return its index |
| `state(nIndex)` | Return the `LuaState` at an index; raises a Ring error for an invalid index |
| `closeState(nIndex)` | Close the state at an index (it stays in the pool) |
| `closeAll()` | Close all the states and empty the pool |
| `count()` | Return the number of states in the pool |
| `doStringAll(cCode)` | Run a Lua code string in every state; returns a list of error messages (`""` on success) |

---

### Lua C API

The Lua C API functions wrapped by the extension can be used directly with the pointer returned by `getPointer()`. Unlike the `LuaState` methods, some of the raw functions are not protected: an error inside them ends the program (Lua panic). See [extensions/ringlua/README.md](extensions/ringlua/README.md) for the list of functions and constants.

---

## Samples

The samples are in the `samples/UsingLuaLib` folder:

| Sample | Description |
|--------|-------------|
| `test01_execute_code.ring` | Run Lua code |
| `test02_ring_to_lua.ring` | Pass variables from Ring to Lua |
| `test03_lua_to_ring.ring` | Get variables from Lua |
| `test04_call_functions.ring` | Call Lua functions with arguments and multiple results |
| `test05_tables_lists.ring` | Exchange lists and tables |
| `test06_error_handling.ring` | Safe methods and catching Lua errors |
| `test07_eval_capture.ring` | Capture the output of `print()` |
| `test08_table_fields.ring` | Read and write table fields |
| `test09_garbage_collection.ring` | Garbage collector and Lua version |
| `test10_multiple_states.ring` | Isolated Lua states |
| `test11_closures.ring` | Fibonacci generator using a Lua closure |
| `test12_string_processing.ring` | Lua string library |
| `test13_math.ring` | Lua math library |
| `test14_nested_tables.ring` | Nested lists and tables |
| `test15_lua_file.ring` | Run and load Lua files |
| `test16_stack_operations.ring` | Push, inspect and pop stack values |
| `test17_quick_functions.ring` | `luaRun()` and `luaEval()` |
| `test18_data_processing.ring` | CSV parsing, report building and pattern matching |
| `test19_game_scripting.ring` | Game entities, combat and inventory scripted in Lua |

---

## Building the Extension

The package includes `ring_lua.dll` for Windows. To build the extension (on Linux and macOS, or after changing `lua.cf`), go to `ring/extensions/ringlua` and run:

| Platform | Commands |
|----------|----------|
| Windows (32-bit) | `gencode.bat` then `buildvc.bat` |
| Windows (64-bit) | `gencode.bat` then `buildvc_x64.bat` |
| Linux | `./gencode.sh` then `./buildgcc.sh` |
| macOS | `./gencode.sh` then `./buildclang.sh` |

The Windows build writes `ring/bin/ring_lua.dll`; the Linux and macOS builds write `libring_lua.so` and `libring_lua.dylib` to `ring/lib`.

To distribute a program using `ring2exe`, add the `-lua` option to copy the extension library.

---

## License

MIT License
