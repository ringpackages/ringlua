#include "ring.h"

/* Copyright (c) 2026 Mahmoud Fayed <msfclipper@yahoo.com> */

/*
**  RingLua - Lua 5.5 Extension for the Ring Programming Language
**  Embeds the Lua scripting language inside Ring applications
*/

#include "lua55/lua.h"
#include "lua55/lualib.h"
#include "lua55/lauxlib.h"

/* Maximum depth when converting nested tables/lists */
#define RING_LUA_MAXDEPTH 64

/* Largest integer a Ring number (double) can hold exactly: 2^53 */
#define RING_LUA_MAXEXACTINT 9007199254740992.0

/* Stack slots needed by one level of table/list conversion */
#define RING_LUA_LEVELSTACK 6

static void ringlua_addvalue ( void *pRingState, List *pList, lua_State *L, int nIndex, int nVisited, int nDepth ) ;

static void ringlua_pushlist ( lua_State *L, List *pList, List **aPath, int nDepth ) ;

/*
**  Push a Ring number: whole numbers become Lua integers (so Lua prints 30, not 30.0)
**  The range is checked before the cast (casting NaN or a huge double to an integer is undefined)
*/
static void ringlua_pushnumber ( lua_State *L, double nValue )
{
	if ( (nValue >= -RING_LUA_MAXEXACTINT) && (nValue <= RING_LUA_MAXEXACTINT) && (nValue == (double) (lua_Integer) nValue) ) {
		lua_pushinteger(L,(lua_Integer) nValue);
	} else {
		lua_pushnumber(L,(lua_Number) nValue);
	}
}

/*
**  Push the Ring list item at nIndex (string, number or nested list) as a Lua value
**  A list that contains itself (using references) is pushed as nil
*/
static void ringlua_pushitem ( lua_State *L, List *pList, int nIndex, List **aPath, int nDepth )
{
	List *pSubList ;
	int x ;
	if ( ring_list_isstring(pList,nIndex) ) {
		lua_pushlstring(L,ring_list_getstring(pList,nIndex),ring_list_getstringsize(pList,nIndex));
		return ;
	}
	if ( ring_list_isnumber(pList,nIndex) ) {
		ringlua_pushnumber(L,ring_list_getdouble(pList,nIndex));
		return ;
	}
	if ( ring_list_islist(pList,nIndex) && (nDepth < RING_LUA_MAXDEPTH) && lua_checkstack(L,RING_LUA_LEVELSTACK) ) {
		pSubList = ring_list_getlist(pList,nIndex);
		for ( x = 1 ; x <= nDepth ; x++ ) {
			if ( aPath[x] == pSubList ) {
				lua_pushnil(L);
				return ;
			}
		}
		ringlua_pushlist(L,pSubList,aPath,nDepth+1);
		return ;
	}
	lua_pushnil(L);
}

/*
**  Push a Ring list as a Lua table
**  Strings and numbers become sequence items {v1, v2, ...}
**  A sub-list of two items whose first item is a string or a number is a [key, value] pair
**  Any other sub-list becomes a nested table
**  aPath holds the lists being converted (1 to nDepth) to detect cycles
*/
static void ringlua_pushlist ( lua_State *L, List *pList, List **aPath, int nDepth )
{
	List *pItem ;
	double nKey ;
	int x, nSize ;
	aPath[nDepth] = pList ;
	nSize = ring_list_getsize(pList);
	lua_createtable(L,nSize,0);
	for ( x = 1 ; x <= nSize ; x++ ) {
		if ( ring_list_islist(pList,x) ) {
			pItem = ring_list_getlist(pList,x);
			if ( (ring_list_getsize(pItem) == 2) && (ring_list_isstring(pItem,1) || ring_list_isnumber(pItem,1)) ) {
				if ( ring_list_isnumber(pItem,1) ) {
					nKey = ring_list_getdouble(pItem,1);
					/* NaN can't be a Lua table key */
					if ( nKey != nKey ) {
						continue;
					}
				}
				ringlua_pushitem(L,pItem,1,aPath,nDepth);
				ringlua_pushitem(L,pItem,2,aPath,nDepth);
				lua_rawset(L,-3);
				continue;
			}
		}
		ringlua_pushitem(L,pList,x,aPath,nDepth);
		lua_rawseti(L,-2,x);
	}
}

/*
**  Push a Ring list as a Lua table (entry point of the conversion)
**  Returns 0 if the Lua stack can't grow
*/
static int ringlua_pushringlist ( lua_State *L, List *pList )
{
	List *aPath[RING_LUA_MAXDEPTH+1] ;
	if ( ! lua_checkstack(L,RING_LUA_LEVELSTACK) ) {
		return 0 ;
	}
	ringlua_pushlist(L,pList,aPath,1);
	return 1 ;
}

/*
**  Add the entries of the Lua table at nIndex to a Ring list as [key, value] pairs
**  nVisited is a Lua table that marks the tables being converted (to detect cycles)
*/
static void ringlua_tabletolist ( void *pRingState, List *pList, lua_State *L, int nIndex, int nVisited, int nDepth )
{
	List *pPair ;
	nIndex = lua_absindex(L,nIndex);
	/* Mark the table */
	lua_pushvalue(L,nIndex);
	lua_pushboolean(L,1);
	lua_rawset(L,nVisited);
	lua_pushnil(L);
	while ( lua_next(L,nIndex) != 0 ) {
		pPair = ring_list_newlist_gc(pRingState,pList);
		ringlua_addvalue(pRingState,pPair,L,-2,nVisited,nDepth);
		ringlua_addvalue(pRingState,pPair,L,-1,nVisited,nDepth);
		/* Pop the value, keep the key for the next iteration */
		lua_pop(L,1);
	}
	/* Unmark the table (a shared sub-table that is not a cycle is converted again) */
	lua_pushvalue(L,nIndex);
	lua_pushnil(L);
	lua_rawset(L,nVisited);
}

/*
**  Add the Lua value at nIndex to a Ring list (tables are converted recursively)
**  A table that contains itself is added as the string "table"
*/
static void ringlua_addvalue ( void *pRingState, List *pList, lua_State *L, int nIndex, int nVisited, int nDepth )
{
	const char *cStr ;
	size_t nSize ;
	int lCycle ;
	nIndex = lua_absindex(L,nIndex);
	switch ( lua_type(L,nIndex) ) {
		case LUA_TSTRING :
			cStr = lua_tolstring(L,nIndex,&nSize);
			ring_list_addstring2_gc(pRingState,pList,cStr,(unsigned int) nSize);
			break ;
		case LUA_TNUMBER :
			if ( lua_isinteger(L,nIndex) ) {
				ring_list_adddouble_gc(pRingState,pList,(double) lua_tointeger(L,nIndex));
			} else {
				ring_list_adddouble_gc(pRingState,pList,(double) lua_tonumber(L,nIndex));
			}
			break ;
		case LUA_TBOOLEAN :
			ring_list_adddouble_gc(pRingState,pList,(double) lua_toboolean(L,nIndex));
			break ;
		case LUA_TNIL :
			ring_list_addstring_gc(pRingState,pList,"nil");
			break ;
		case LUA_TTABLE :
			if ( (nDepth < RING_LUA_MAXDEPTH) && lua_checkstack(L,RING_LUA_LEVELSTACK) ) {
				lua_pushvalue(L,nIndex);
				lua_rawget(L,nVisited);
				lCycle = lua_toboolean(L,-1);
				lua_pop(L,1);
				if ( ! lCycle ) {
					ringlua_tabletolist(pRingState,ring_list_newlist_gc(pRingState,pList),L,nIndex,nVisited,nDepth+1);
					break ;
				}
			}
			ring_list_addstring_gc(pRingState,pList,"table");
			break ;
		default :
			ring_list_addstring_gc(pRingState,pList,lua_typename(L,lua_type(L,nIndex)));
			break ;
	}
}

/*
**  Protected access to globals and table fields
**  lua_getglobal(), lua_setglobal(), lua_getfield() and lua_setfield() may call metamethods
**  An error outside a protected call ends the program (Lua panic), so they run using lua_pcall()
*/
static int ringlua_getglobal_k ( lua_State *L )
{
	/* 1: name */
	lua_getglobal(L,lua_tostring(L,1));
	return 1 ;
}

static int ringlua_setglobal_k ( lua_State *L )
{
	/* 1: value, 2: name */
	lua_pushvalue(L,1);
	lua_setglobal(L,lua_tostring(L,2));
	return 0 ;
}

static int ringlua_getfield_k ( lua_State *L )
{
	/* 1: table, 2: key */
	lua_getfield(L,1,lua_tostring(L,2));
	return 1 ;
}

static int ringlua_setfield_k ( lua_State *L )
{
	/* 1: value, 2: table, 3: key */
	lua_pushvalue(L,1);
	lua_setfield(L,2,lua_tostring(L,3));
	return 0 ;
}

/* Push the global (or the error message) and return the status */
static int ringlua_pgetglobal ( lua_State *L, const char *cName )
{
	lua_pushcfunction(L,ringlua_getglobal_k);
	lua_pushstring(L,cName);
	return lua_pcall(L,1,1,0) ;
}

/* Pop the value on the top and set the global; on error push the error message */
static int ringlua_psetglobal ( lua_State *L, const char *cName )
{
	lua_pushcfunction(L,ringlua_setglobal_k);
	lua_insert(L,-2);
	lua_pushstring(L,cName);
	return lua_pcall(L,2,0,0) ;
}

/* Push t[k] for the table at nIndex (or the error message) and return the status */
static int ringlua_pgetfield ( lua_State *L, int nIndex, const char *cKey )
{
	nIndex = lua_absindex(L,nIndex);
	lua_pushcfunction(L,ringlua_getfield_k);
	lua_pushvalue(L,nIndex);
	lua_pushstring(L,cKey);
	return lua_pcall(L,2,1,0) ;
}

/* Pop the value on the top and set t[k] for the table at nIndex; on error push the error message */
static int ringlua_psetfield ( lua_State *L, int nIndex, const char *cKey )
{
	nIndex = lua_absindex(L,nIndex);
	lua_pushcfunction(L,ringlua_setfield_k);
	lua_insert(L,-2);
	lua_pushvalue(L,nIndex);
	lua_pushstring(L,cKey);
	return lua_pcall(L,3,0,0) ;
}

RING_FUNC(ring_get_lua_multret)
{
	RING_API_RETNUMBER(LUA_MULTRET);
}

RING_FUNC(ring_get_lua_registryindex)
{
	RING_API_RETNUMBER(LUA_REGISTRYINDEX);
}

RING_FUNC(ring_get_lua_ok)
{
	RING_API_RETNUMBER(LUA_OK);
}

RING_FUNC(ring_get_lua_yield)
{
	RING_API_RETNUMBER(LUA_YIELD);
}

RING_FUNC(ring_get_lua_errrun)
{
	RING_API_RETNUMBER(LUA_ERRRUN);
}

RING_FUNC(ring_get_lua_errsyntax)
{
	RING_API_RETNUMBER(LUA_ERRSYNTAX);
}

RING_FUNC(ring_get_lua_errmem)
{
	RING_API_RETNUMBER(LUA_ERRMEM);
}

RING_FUNC(ring_get_lua_errerr)
{
	RING_API_RETNUMBER(LUA_ERRERR);
}

RING_FUNC(ring_get_lua_tnone)
{
	RING_API_RETNUMBER(LUA_TNONE);
}

RING_FUNC(ring_get_lua_tnil)
{
	RING_API_RETNUMBER(LUA_TNIL);
}

RING_FUNC(ring_get_lua_tboolean)
{
	RING_API_RETNUMBER(LUA_TBOOLEAN);
}

RING_FUNC(ring_get_lua_tlightuserdata)
{
	RING_API_RETNUMBER(LUA_TLIGHTUSERDATA);
}

RING_FUNC(ring_get_lua_tnumber)
{
	RING_API_RETNUMBER(LUA_TNUMBER);
}

RING_FUNC(ring_get_lua_tstring)
{
	RING_API_RETNUMBER(LUA_TSTRING);
}

RING_FUNC(ring_get_lua_ttable)
{
	RING_API_RETNUMBER(LUA_TTABLE);
}

RING_FUNC(ring_get_lua_tfunction)
{
	RING_API_RETNUMBER(LUA_TFUNCTION);
}

RING_FUNC(ring_get_lua_tuserdata)
{
	RING_API_RETNUMBER(LUA_TUSERDATA);
}

RING_FUNC(ring_get_lua_tthread)
{
	RING_API_RETNUMBER(LUA_TTHREAD);
}

RING_FUNC(ring_get_lua_numtypes)
{
	RING_API_RETNUMBER(LUA_NUMTYPES);
}

RING_FUNC(ring_get_lua_minstack)
{
	RING_API_RETNUMBER(LUA_MINSTACK);
}

RING_FUNC(ring_get_lua_ridx_globals)
{
	RING_API_RETNUMBER(LUA_RIDX_GLOBALS);
}

RING_FUNC(ring_get_lua_ridx_mainthread)
{
	RING_API_RETNUMBER(LUA_RIDX_MAINTHREAD);
}

RING_FUNC(ring_get_lua_opadd)
{
	RING_API_RETNUMBER(LUA_OPADD);
}

RING_FUNC(ring_get_lua_opsub)
{
	RING_API_RETNUMBER(LUA_OPSUB);
}

RING_FUNC(ring_get_lua_opmul)
{
	RING_API_RETNUMBER(LUA_OPMUL);
}

RING_FUNC(ring_get_lua_opmod)
{
	RING_API_RETNUMBER(LUA_OPMOD);
}

RING_FUNC(ring_get_lua_oppow)
{
	RING_API_RETNUMBER(LUA_OPPOW);
}

RING_FUNC(ring_get_lua_opdiv)
{
	RING_API_RETNUMBER(LUA_OPDIV);
}

RING_FUNC(ring_get_lua_opidiv)
{
	RING_API_RETNUMBER(LUA_OPIDIV);
}

RING_FUNC(ring_get_lua_opband)
{
	RING_API_RETNUMBER(LUA_OPBAND);
}

RING_FUNC(ring_get_lua_opbor)
{
	RING_API_RETNUMBER(LUA_OPBOR);
}

RING_FUNC(ring_get_lua_opbxor)
{
	RING_API_RETNUMBER(LUA_OPBXOR);
}

RING_FUNC(ring_get_lua_opshl)
{
	RING_API_RETNUMBER(LUA_OPSHL);
}

RING_FUNC(ring_get_lua_opshr)
{
	RING_API_RETNUMBER(LUA_OPSHR);
}

RING_FUNC(ring_get_lua_opunm)
{
	RING_API_RETNUMBER(LUA_OPUNM);
}

RING_FUNC(ring_get_lua_opbnot)
{
	RING_API_RETNUMBER(LUA_OPBNOT);
}

RING_FUNC(ring_get_lua_opeq)
{
	RING_API_RETNUMBER(LUA_OPEQ);
}

RING_FUNC(ring_get_lua_oplt)
{
	RING_API_RETNUMBER(LUA_OPLT);
}

RING_FUNC(ring_get_lua_ople)
{
	RING_API_RETNUMBER(LUA_OPLE);
}

RING_FUNC(ring_get_lua_gcstop)
{
	RING_API_RETNUMBER(LUA_GCSTOP);
}

RING_FUNC(ring_get_lua_gcrestart)
{
	RING_API_RETNUMBER(LUA_GCRESTART);
}

RING_FUNC(ring_get_lua_gccollect)
{
	RING_API_RETNUMBER(LUA_GCCOLLECT);
}

RING_FUNC(ring_get_lua_gccount)
{
	RING_API_RETNUMBER(LUA_GCCOUNT);
}

RING_FUNC(ring_get_lua_gccountb)
{
	RING_API_RETNUMBER(LUA_GCCOUNTB);
}

RING_FUNC(ring_get_lua_gcstep)
{
	RING_API_RETNUMBER(LUA_GCSTEP);
}

RING_FUNC(ring_get_lua_gcisrunning)
{
	RING_API_RETNUMBER(LUA_GCISRUNNING);
}

RING_FUNC(ring_get_lua_gcgen)
{
	RING_API_RETNUMBER(LUA_GCGEN);
}

RING_FUNC(ring_get_lua_gcinc)
{
	RING_API_RETNUMBER(LUA_GCINC);
}

RING_FUNC(ring_get_lua_gcparam)
{
	RING_API_RETNUMBER(LUA_GCPARAM);
}

RING_FUNC(ring_get_lua_hookcall)
{
	RING_API_RETNUMBER(LUA_HOOKCALL);
}

RING_FUNC(ring_get_lua_hookret)
{
	RING_API_RETNUMBER(LUA_HOOKRET);
}

RING_FUNC(ring_get_lua_hookline)
{
	RING_API_RETNUMBER(LUA_HOOKLINE);
}

RING_FUNC(ring_get_lua_hookcount)
{
	RING_API_RETNUMBER(LUA_HOOKCOUNT);
}

RING_FUNC(ring_get_lua_hooktailcall)
{
	RING_API_RETNUMBER(LUA_HOOKTAILCALL);
}

RING_FUNC(ring_get_lua_maskcall)
{
	RING_API_RETNUMBER(LUA_MASKCALL);
}

RING_FUNC(ring_get_lua_maskret)
{
	RING_API_RETNUMBER(LUA_MASKRET);
}

RING_FUNC(ring_get_lua_maskline)
{
	RING_API_RETNUMBER(LUA_MASKLINE);
}

RING_FUNC(ring_get_lua_maskcount)
{
	RING_API_RETNUMBER(LUA_MASKCOUNT);
}


RING_FUNC(ring_lua_version)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_version((lua_State *) RING_API_GETCPOINTER(1,"lua_State")));
}


RING_FUNC(ring_lua_status)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_status((lua_State *) RING_API_GETCPOINTER(1,"lua_State")));
}


RING_FUNC(ring_lua_absindex)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_absindex((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_gettop)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gettop((lua_State *) RING_API_GETCPOINTER(1,"lua_State")));
}


RING_FUNC(ring_lua_settop)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_settop((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_pop)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_pop((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_pushvalue)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_pushvalue((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_rotate)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_rotate((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_lua_copy)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_copy((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_lua_checkstack)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_checkstack((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_xmove)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISCPOINTER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_xmove((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),(lua_State *) RING_API_GETCPOINTER(2,"lua_State"), (int ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_lua_isnumber)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isnumber((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isstring)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isstring((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_iscfunction)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_iscfunction((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isinteger)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isinteger((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isuserdata)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isuserdata((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_type)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_type((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_typename)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETSTRING(lua_typename((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_tonumberx)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_tonumberx((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2),RING_API_GETINTPOINTER(3)));
	RING_API_ACCEPTINTVALUE(3) ;
}


RING_FUNC(ring_lua_tointegerx)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_tointegerx((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2),RING_API_GETINTPOINTER(3)));
	RING_API_ACCEPTINTVALUE(3) ;
}


RING_FUNC(ring_lua_toboolean)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_toboolean((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_rawlen)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_rawlen((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_tonumber)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_tonumber((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_tointeger)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_tointeger((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isfunction)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isfunction((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_istable)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_istable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isnil)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isnil((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isboolean)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isboolean((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isnone)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isnone((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_isnoneornil)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isnoneornil((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_pushnil)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_pushnil((lua_State *) RING_API_GETCPOINTER(1,"lua_State"));
}


RING_FUNC(ring_lua_pushnumber)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_pushnumber((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (lua_Number ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_pushinteger)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_pushinteger((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (lua_Integer ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_pushstring)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETSTRING(lua_pushstring((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2)));
}


RING_FUNC(ring_lua_pushlstring)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETSTRING(lua_pushlstring((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2), (size_t ) RING_API_GETNUMBER(3)));
}


RING_FUNC(ring_lua_pushboolean)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_pushboolean((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_arith)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_arith((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_rawequal)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_rawequal((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3)));
}


RING_FUNC(ring_lua_compare)
{
	if ( RING_API_PARACOUNT != 4 ) {
		RING_API_ERROR(RING_API_MISS4PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(4) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_compare((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3), (int ) RING_API_GETNUMBER(4)));
}


RING_FUNC(ring_lua_getglobal)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_getglobal((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2)));
}


RING_FUNC(ring_lua_gettable)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gettable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_getfield)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_getfield((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2),RING_API_GETSTRING(3)));
}


RING_FUNC(ring_lua_geti)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_geti((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (lua_Integer ) RING_API_GETNUMBER(3)));
}


RING_FUNC(ring_lua_rawget)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_rawget((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_rawgeti)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_rawgeti((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (lua_Integer ) RING_API_GETNUMBER(3)));
}


RING_FUNC(ring_lua_createtable)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_createtable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_lua_getmetatable)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_getmetatable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_setglobal)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_setglobal((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2));
}


RING_FUNC(ring_lua_settable)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_settable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_setfield)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_setfield((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2),RING_API_GETSTRING(3));
}


RING_FUNC(ring_lua_seti)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_seti((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (lua_Integer ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_lua_rawset)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_rawset((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_rawseti)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_rawseti((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (lua_Integer ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_lua_setmetatable)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_setmetatable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_call)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_call((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_lua_pcall)
{
	if ( RING_API_PARACOUNT != 4 ) {
		RING_API_ERROR(RING_API_MISS4PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(4) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_pcall((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3), (int ) RING_API_GETNUMBER(4)));
}


RING_FUNC(ring_lua_error)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_error((lua_State *) RING_API_GETCPOINTER(1,"lua_State")));
}


RING_FUNC(ring_lua_next)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_next((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_lua_concat)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_concat((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_len)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_len((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_stringtonumber)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_stringtonumber((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2)));
}


RING_FUNC(ring_lua_toclose)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_toclose((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_closeslot)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	lua_closeslot((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_lua_isyieldable)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_isyieldable((lua_State *) RING_API_GETCPOINTER(1,"lua_State")));
}


RING_FUNC(ring_luaL_getmetafield)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_getmetafield((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2),RING_API_GETSTRING(3)));
}


RING_FUNC(ring_luaL_callmeta)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_callmeta((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2),RING_API_GETSTRING(3)));
}


RING_FUNC(ring_luaL_newmetatable)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_newmetatable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2)));
}


RING_FUNC(ring_luaL_setmetatable)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	luaL_setmetatable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2));
}


RING_FUNC(ring_luaL_where)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	luaL_where((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2));
}


RING_FUNC(ring_luaL_ref)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_ref((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_luaL_unref)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	luaL_unref((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3));
}


RING_FUNC(ring_luaL_loadfilex)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_loadfilex((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2),RING_API_GETSTRING(3)));
}


RING_FUNC(ring_luaL_loadstring)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_loadstring((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2)));
}


RING_FUNC(ring_luaL_len)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_len((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2)));
}


RING_FUNC(ring_luaL_gsub)
{
	if ( RING_API_PARACOUNT != 4 ) {
		RING_API_ERROR(RING_API_MISS4PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(4) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETSTRING(luaL_gsub((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),RING_API_GETSTRING(2),RING_API_GETSTRING(3),RING_API_GETSTRING(4)));
}


RING_FUNC(ring_luaL_getsubtable)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(luaL_getsubtable((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2),RING_API_GETSTRING(3)));
}


RING_FUNC(ring_luaL_traceback)
{
	if ( RING_API_PARACOUNT != 4 ) {
		RING_API_ERROR(RING_API_MISS4PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISCPOINTER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(4) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	luaL_traceback((lua_State *) RING_API_GETCPOINTER(1,"lua_State"),(lua_State *) RING_API_GETCPOINTER(2,"lua_State"),RING_API_GETSTRING(3), (int ) RING_API_GETNUMBER(4));
}


RING_FUNC(ring_luaL_openselectedlibs)
{
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	luaL_openselectedlibs((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), (int ) RING_API_GETNUMBER(2), (int ) RING_API_GETNUMBER(3));
}


/* ========================================================================== */
/*  Custom Ring Function Implementations                                      */
/* ========================================================================== */

/*
** Free function of the managed lua_State pointer.
** The Ring VM calls it when the last reference to the pointer is deleted
** (it is not called after lua_close(), because lua_close() sets the pointer to NULL).
*/
static void ringlua_freestate ( void *pRingState, void *pState )
{
	lua_close((lua_State *) pState);
}

/* --- luaL_newstate: returns a managed pointer (the state is closed when no variable refers to it) --- */
RING_FUNC(ring_luaL_newstate)
{
	lua_State *L;
	if ( RING_API_PARACOUNT != 0 ) {
		RING_API_ERROR(RING_API_BADPARACOUNT);
		return ;
	}
	L = luaL_newstate();
	if ( L == NULL ) {
		RING_API_ERROR("Error: Failed to create Lua state");
		return ;
	}
	RING_API_RETMANAGEDCPOINTER(L,"lua_State",ringlua_freestate);
}

/*
** lua_close: Close the state and set the pointer to NULL.
** Copies of the pointer share the same item, so they become NULL too,
** and the Ring VM doesn't call the free function again.
** Closing a NULL pointer does nothing.
*/
RING_FUNC(ring_lua_close)
{
	List *pList;
	lua_State *L;
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	pList = RING_API_GETLIST(1);
	L = (lua_State *) ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if ( L == NULL ) {
		return ;
	}
	if ( strcmp(ring_list_getstring(pList, RING_CPOINTER_TYPE), "lua_State") != 0 ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	ring_list_setpointer_gc(RING_API_STATE, pList, RING_CPOINTER_POINTER, NULL);
	lua_close(L);
}

/* --- lua_newthread --- */
RING_FUNC(ring_lua_newthread)
{
	lua_State *L2;
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L2 = lua_newthread((lua_State *) RING_API_GETCPOINTER(1,"lua_State"));
	RING_API_RETCPOINTER(L2,"lua_State");
}

/* --- lua_closethread --- */
RING_FUNC(ring_lua_closethread)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISCPOINTER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER( lua_closethread(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		(lua_State *) RING_API_GETCPOINTER(2,"lua_State")
	));
}

/* --- lua_tolstring: returns string with length --- */
RING_FUNC(ring_lua_tolstring)
{
	size_t len = 0;
	const char *str;
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	str = lua_tolstring(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		(int) RING_API_GETNUMBER(2), &len
	);
	if ( str == NULL ) {
		RING_API_RETSTRING("");
	} else {
		RING_API_RETSTRING2((char *)str, len);
	}
}

/* --- luaL_tolstring: converts any value to string --- */
RING_FUNC(ring_luaL_tolstring)
{
	size_t len = 0;
	const char *str;
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	str = luaL_tolstring(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		(int) RING_API_GETNUMBER(2), &len
	);
	if ( str == NULL ) {
		RING_API_RETSTRING("");
	} else {
		RING_API_RETSTRING2((char *)str, len);
	}
	lua_pop((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), 1);
}

/* --- luaL_checklstring --- */
RING_FUNC(ring_luaL_checklstring)
{
	size_t len = 0;
	const char *str;
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	str = luaL_checklstring(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		(int) RING_API_GETNUMBER(2), &len
	);
	if ( str == NULL ) {
		RING_API_RETSTRING("");
	} else {
		RING_API_RETSTRING2((char *)str, len);
	}
}

/*
** Run the chunk on the top of the stack (or the load error) and return the error message, or "" on success.
** The stack is restored to nTop (values returned by the chunk are removed).
*/
static void ringlua_runchunk ( void *pPointer, lua_State *L, int nTop, int nResult )
{
	const char *errmsg ;
	if ( nResult == LUA_OK ) {
		nResult = lua_pcall(L, 0, LUA_MULTRET, 0);
	}
	if ( nResult != LUA_OK ) {
		errmsg = lua_tostring(L, -1);
		if ( errmsg != NULL ) {
			RING_API_RETSTRING((char *)errmsg);
		} else {
			RING_API_RETSTRING("Unknown Lua error");
		}
	} else {
		RING_API_RETSTRING("");
	}
	lua_settop(L, nTop);
}

/* --- luaL_dostring with error message support --- */
RING_FUNC(ring_luaL_dostring_errormsg)
{
	lua_State *L;
	int nTop;
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	nTop = lua_gettop(L);
	ringlua_runchunk(pPointer, L, nTop, luaL_loadstring(L, RING_API_GETSTRING(2)));
}

/* --- luaL_dofile with error message support --- */
RING_FUNC(ring_luaL_dofile_errormsg)
{
	lua_State *L;
	int nTop;
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	nTop = lua_gettop(L);
	ringlua_runchunk(pPointer, L, nTop, luaL_loadfile(L, RING_API_GETSTRING(2)));
}

/* --- Garbage Collection wrapper functions --- */
RING_FUNC(ring_lua_gc_collect)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gc(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		LUA_GCCOLLECT
	));
}

RING_FUNC(ring_lua_gc_stop)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gc(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		LUA_GCSTOP
	));
}

RING_FUNC(ring_lua_gc_restart)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gc(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		LUA_GCRESTART
	));
}

RING_FUNC(ring_lua_gc_count)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gc(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		LUA_GCCOUNT
	));
}

RING_FUNC(ring_lua_gc_step)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gc(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		LUA_GCSTEP, (int) RING_API_GETNUMBER(2)
	));
}

RING_FUNC(ring_lua_gc_isrunning)
{
	if ( RING_API_PARACOUNT != 1 ) {
		RING_API_ERROR(RING_API_MISS1PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	RING_API_RETNUMBER(lua_gc(
		(lua_State *) RING_API_GETCPOINTER(1,"lua_State"),
		LUA_GCISRUNNING
	));
}

/* ========================================================================== */
/*  Lua Table <-> Ring List Conversion                                        */
/* ========================================================================== */

/*
** lua_toring_table: Convert a Lua table at the given stack index to a Ring List.
** Lua tables are converted to Ring lists of [key,value] pairs.
** Nested tables are recursively converted (a table that contains itself becomes "table").
*/
RING_FUNC(ring_lua_toring_table)
{
	lua_State *L;
	int idx, nVisited;
	List *pList;

	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}

	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	idx = lua_absindex(L, (int) RING_API_GETNUMBER(2));

	if ( !lua_istable(L, idx) ) {
		RING_API_ERROR("Error: Expected a Lua table");
		return ;
	}
	if ( ! lua_checkstack(L, RING_LUA_LEVELSTACK) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}

	/* Table of the tables being converted (to detect cycles) */
	lua_newtable(L);
	nVisited = lua_gettop(L);
	pList = RING_API_NEWLIST ;
	ringlua_tabletolist(RING_API_STATE, pList, L, idx, nVisited, 1);
	lua_pop(L, 1);
	RING_API_RETLIST(pList);
}

/*
** lua_pushring_table: Push a Ring List as a Lua table.
** Strings and numbers become sequence items {v1, v2, ...}.
** A sub-list of two items whose first item is a string or a number is a [key, value] pair.
** Any other sub-list becomes a nested table (a list that contains itself becomes nil).
*/
RING_FUNC(ring_lua_pushring_table)
{
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISLIST(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! ringlua_pushringlist((lua_State *) RING_API_GETCPOINTER(1,"lua_State"), RING_API_GETLIST(2)) ) {
		RING_API_ERROR("Error: Lua stack overflow");
	}
}

/*
** Set the value on the top of the stack as a Lua global (raises a Ring error on failure)
*/
static void ringlua_setglobalvalue ( void *pPointer, lua_State *L, const char *cName )
{
	if ( ringlua_psetglobal(L, cName) != LUA_OK ) {
		RING_API_ERROR(lua_tostring(L, -1) != NULL ? lua_tostring(L, -1) : "Unknown Lua error");
		lua_pop(L, 1);
	}
}

/*
** lua_pushring_variable: Set a Ring string as a Lua global variable.
*/
RING_FUNC(ring_lua_pushring_variable)
{
	lua_State *L;
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	if ( ! lua_checkstack(L, 4) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}
	lua_pushlstring(L, RING_API_GETSTRING(3), RING_API_GETSTRINGSIZE(3));
	ringlua_setglobalvalue(pPointer, L, RING_API_GETSTRING(2));
}

/*
** lua_pushring_number: Set a Ring number as a Lua global variable.
*/
RING_FUNC(ring_lua_pushring_number)
{
	lua_State *L;
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	if ( ! lua_checkstack(L, 4) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}
	ringlua_pushnumber(L, RING_API_GETNUMBER(3));
	ringlua_setglobalvalue(pPointer, L, RING_API_GETSTRING(2));
}

/*
** lua_callring_function: Call a Lua function by name with arguments from a Ring List.
** Arguments list items can be strings, numbers or lists (pushed as tables).
** Returns: the stack size after the call (the results are on the top), or -1 on error.
** On error, the error message is on the top of the Lua stack.
*/
RING_FUNC(ring_lua_callring_function)
{
	lua_State *L;
	List *pArgs;
	List *aPath[RING_LUA_MAXDEPTH+1];
	int nArgs, i;

	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISLIST(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}

	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	pArgs = RING_API_GETLIST(3);
	nArgs = ring_list_getsize(pArgs);

	/* Room for the function, the arguments and the conversion of nested lists */
	if ( ! lua_checkstack(L, nArgs + RING_LUA_LEVELSTACK) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}

	/* Get the function from global scope */
	if ( ringlua_pgetglobal(L, RING_API_GETSTRING(2)) != LUA_OK ) {
		RING_API_RETNUMBER(-1);
		return ;
	}
	if ( !lua_isfunction(L, -1) ) {
		lua_pop(L, 1);
		lua_pushfstring(L, "'%s' is not a function", RING_API_GETSTRING(2));
		RING_API_RETNUMBER(-1);
		return ;
	}

	/* Push arguments */
	aPath[1] = pArgs;
	for ( i = 1 ; i <= nArgs ; i++ ) {
		ringlua_pushitem(L, pArgs, i, aPath, 1);
	}

	/* Call the function */
	if ( lua_pcall(L, nArgs, LUA_MULTRET, 0) != LUA_OK ) {
		RING_API_RETNUMBER(-1);
	} else {
		RING_API_RETNUMBER(lua_gettop(L));
	}
}

/*
** Protected access to globals and table fields (metamethods may raise errors).
** Each function returns LUA_OK on success; on error the error message is pushed.
** lua_pgetglobal() and lua_pgetfield() push the value on success.
** lua_psetglobal() and lua_psetfield() pop the value on the top of the stack.
*/
RING_FUNC(ring_lua_pgetglobal)
{
	lua_State *L;
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	if ( ! lua_checkstack(L, 4) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}
	RING_API_RETNUMBER(ringlua_pgetglobal(L, RING_API_GETSTRING(2)));
}

RING_FUNC(ring_lua_psetglobal)
{
	lua_State *L;
	if ( RING_API_PARACOUNT != 2 ) {
		RING_API_ERROR(RING_API_MISS2PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	if ( lua_gettop(L) < 1 ) {
		RING_API_ERROR("Error: The Lua stack is empty");
		return ;
	}
	if ( ! lua_checkstack(L, 4) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}
	RING_API_RETNUMBER(ringlua_psetglobal(L, RING_API_GETSTRING(2)));
}

RING_FUNC(ring_lua_pgetfield)
{
	lua_State *L;
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	if ( lua_isnone(L, (int) RING_API_GETNUMBER(2)) ) {
		RING_API_ERROR("Error: Invalid Lua stack index");
		return ;
	}
	if ( ! lua_checkstack(L, 4) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}
	RING_API_RETNUMBER(ringlua_pgetfield(L, (int) RING_API_GETNUMBER(2), RING_API_GETSTRING(3)));
}

RING_FUNC(ring_lua_psetfield)
{
	lua_State *L;
	if ( RING_API_PARACOUNT != 3 ) {
		RING_API_ERROR(RING_API_MISS3PARA);
		return ;
	}
	RING_API_IGNORECPOINTERTYPE ;
	if ( ! RING_API_ISCPOINTER(1) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISNUMBER(2) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	if ( ! RING_API_ISSTRING(3) ) {
		RING_API_ERROR(RING_API_BADPARATYPE);
		return ;
	}
	L = (lua_State *) RING_API_GETCPOINTER(1,"lua_State");
	if ( (lua_gettop(L) < 2) || lua_isnone(L, (int) RING_API_GETNUMBER(2)) ) {
		RING_API_ERROR("Error: Invalid Lua stack index");
		return ;
	}
	if ( ! lua_checkstack(L, 4) ) {
		RING_API_ERROR("Error: Lua stack overflow");
		return ;
	}
	RING_API_RETNUMBER(ringlua_psetfield(L, (int) RING_API_GETNUMBER(2), RING_API_GETSTRING(3)));
}

RING_LIBINIT
{
	RING_API_REGISTER("lua_version",ring_lua_version);
	RING_API_REGISTER("lua_status",ring_lua_status);
	RING_API_REGISTER("lua_absindex",ring_lua_absindex);
	RING_API_REGISTER("lua_gettop",ring_lua_gettop);
	RING_API_REGISTER("lua_settop",ring_lua_settop);
	RING_API_REGISTER("lua_pop",ring_lua_pop);
	RING_API_REGISTER("lua_pushvalue",ring_lua_pushvalue);
	RING_API_REGISTER("lua_rotate",ring_lua_rotate);
	RING_API_REGISTER("lua_copy",ring_lua_copy);
	RING_API_REGISTER("lua_checkstack",ring_lua_checkstack);
	RING_API_REGISTER("lua_xmove",ring_lua_xmove);
	RING_API_REGISTER("lua_isnumber",ring_lua_isnumber);
	RING_API_REGISTER("lua_isstring",ring_lua_isstring);
	RING_API_REGISTER("lua_iscfunction",ring_lua_iscfunction);
	RING_API_REGISTER("lua_isinteger",ring_lua_isinteger);
	RING_API_REGISTER("lua_isuserdata",ring_lua_isuserdata);
	RING_API_REGISTER("lua_type",ring_lua_type);
	RING_API_REGISTER("lua_typename",ring_lua_typename);
	RING_API_REGISTER("lua_tonumberx",ring_lua_tonumberx);
	RING_API_REGISTER("lua_tointegerx",ring_lua_tointegerx);
	RING_API_REGISTER("lua_toboolean",ring_lua_toboolean);
	RING_API_REGISTER("lua_rawlen",ring_lua_rawlen);
	RING_API_REGISTER("lua_tonumber",ring_lua_tonumber);
	RING_API_REGISTER("lua_tointeger",ring_lua_tointeger);
	RING_API_REGISTER("lua_isfunction",ring_lua_isfunction);
	RING_API_REGISTER("lua_istable",ring_lua_istable);
	RING_API_REGISTER("lua_isnil",ring_lua_isnil);
	RING_API_REGISTER("lua_isboolean",ring_lua_isboolean);
	RING_API_REGISTER("lua_isnone",ring_lua_isnone);
	RING_API_REGISTER("lua_isnoneornil",ring_lua_isnoneornil);
	RING_API_REGISTER("lua_pushnil",ring_lua_pushnil);
	RING_API_REGISTER("lua_pushnumber",ring_lua_pushnumber);
	RING_API_REGISTER("lua_pushinteger",ring_lua_pushinteger);
	RING_API_REGISTER("lua_pushstring",ring_lua_pushstring);
	RING_API_REGISTER("lua_pushlstring",ring_lua_pushlstring);
	RING_API_REGISTER("lua_pushboolean",ring_lua_pushboolean);
	RING_API_REGISTER("lua_arith",ring_lua_arith);
	RING_API_REGISTER("lua_rawequal",ring_lua_rawequal);
	RING_API_REGISTER("lua_compare",ring_lua_compare);
	RING_API_REGISTER("lua_getglobal",ring_lua_getglobal);
	RING_API_REGISTER("lua_gettable",ring_lua_gettable);
	RING_API_REGISTER("lua_getfield",ring_lua_getfield);
	RING_API_REGISTER("lua_geti",ring_lua_geti);
	RING_API_REGISTER("lua_rawget",ring_lua_rawget);
	RING_API_REGISTER("lua_rawgeti",ring_lua_rawgeti);
	RING_API_REGISTER("lua_createtable",ring_lua_createtable);
	RING_API_REGISTER("lua_getmetatable",ring_lua_getmetatable);
	RING_API_REGISTER("lua_setglobal",ring_lua_setglobal);
	RING_API_REGISTER("lua_settable",ring_lua_settable);
	RING_API_REGISTER("lua_setfield",ring_lua_setfield);
	RING_API_REGISTER("lua_seti",ring_lua_seti);
	RING_API_REGISTER("lua_rawset",ring_lua_rawset);
	RING_API_REGISTER("lua_rawseti",ring_lua_rawseti);
	RING_API_REGISTER("lua_setmetatable",ring_lua_setmetatable);
	RING_API_REGISTER("lua_call",ring_lua_call);
	RING_API_REGISTER("lua_pcall",ring_lua_pcall);
	RING_API_REGISTER("lua_error",ring_lua_error);
	RING_API_REGISTER("lua_next",ring_lua_next);
	RING_API_REGISTER("lua_concat",ring_lua_concat);
	RING_API_REGISTER("lua_len",ring_lua_len);
	RING_API_REGISTER("lua_stringtonumber",ring_lua_stringtonumber);
	RING_API_REGISTER("lua_toclose",ring_lua_toclose);
	RING_API_REGISTER("lua_closeslot",ring_lua_closeslot);
	RING_API_REGISTER("lua_isyieldable",ring_lua_isyieldable);
	RING_API_REGISTER("lual_getmetafield",ring_luaL_getmetafield);
	RING_API_REGISTER("lual_callmeta",ring_luaL_callmeta);
	RING_API_REGISTER("lual_newmetatable",ring_luaL_newmetatable);
	RING_API_REGISTER("lual_setmetatable",ring_luaL_setmetatable);
	RING_API_REGISTER("lual_where",ring_luaL_where);
	RING_API_REGISTER("lual_ref",ring_luaL_ref);
	RING_API_REGISTER("lual_unref",ring_luaL_unref);
	RING_API_REGISTER("lual_loadfilex",ring_luaL_loadfilex);
	RING_API_REGISTER("lual_loadstring",ring_luaL_loadstring);
	RING_API_REGISTER("lual_len",ring_luaL_len);
	RING_API_REGISTER("lual_gsub",ring_luaL_gsub);
	RING_API_REGISTER("lual_getsubtable",ring_luaL_getsubtable);
	RING_API_REGISTER("lual_traceback",ring_luaL_traceback);
	RING_API_REGISTER("lual_openselectedlibs",ring_luaL_openselectedlibs);
	RING_API_REGISTER("lual_newstate",ring_luaL_newstate);
	RING_API_REGISTER("lua_close",ring_lua_close);
	RING_API_REGISTER("lua_newthread",ring_lua_newthread);
	RING_API_REGISTER("lua_closethread",ring_lua_closethread);
	RING_API_REGISTER("lua_tolstring",ring_lua_tolstring);
	RING_API_REGISTER("lual_tolstring",ring_luaL_tolstring);
	RING_API_REGISTER("lual_checklstring",ring_luaL_checklstring);
	RING_API_REGISTER("lual_dostring_errormsg",ring_luaL_dostring_errormsg);
	RING_API_REGISTER("lual_dofile_errormsg",ring_luaL_dofile_errormsg);
	RING_API_REGISTER("lua_gc_collect",ring_lua_gc_collect);
	RING_API_REGISTER("lua_gc_stop",ring_lua_gc_stop);
	RING_API_REGISTER("lua_gc_restart",ring_lua_gc_restart);
	RING_API_REGISTER("lua_gc_count",ring_lua_gc_count);
	RING_API_REGISTER("lua_gc_step",ring_lua_gc_step);
	RING_API_REGISTER("lua_gc_isrunning",ring_lua_gc_isrunning);
	RING_API_REGISTER("lua_toring_table",ring_lua_toring_table);
	RING_API_REGISTER("lua_pushring_table",ring_lua_pushring_table);
	RING_API_REGISTER("lua_pushring_variable",ring_lua_pushring_variable);
	RING_API_REGISTER("lua_pushring_number",ring_lua_pushring_number);
	RING_API_REGISTER("lua_callring_function",ring_lua_callring_function);
	RING_API_REGISTER("lua_pgetglobal",ring_lua_pgetglobal);
	RING_API_REGISTER("lua_psetglobal",ring_lua_psetglobal);
	RING_API_REGISTER("lua_pgetfield",ring_lua_pgetfield);
	RING_API_REGISTER("lua_psetfield",ring_lua_psetfield);
	RING_API_REGISTER("get_lua_multret",ring_get_lua_multret);
	RING_API_REGISTER("get_lua_registryindex",ring_get_lua_registryindex);
	RING_API_REGISTER("get_lua_ok",ring_get_lua_ok);
	RING_API_REGISTER("get_lua_yield",ring_get_lua_yield);
	RING_API_REGISTER("get_lua_errrun",ring_get_lua_errrun);
	RING_API_REGISTER("get_lua_errsyntax",ring_get_lua_errsyntax);
	RING_API_REGISTER("get_lua_errmem",ring_get_lua_errmem);
	RING_API_REGISTER("get_lua_errerr",ring_get_lua_errerr);
	RING_API_REGISTER("get_lua_tnone",ring_get_lua_tnone);
	RING_API_REGISTER("get_lua_tnil",ring_get_lua_tnil);
	RING_API_REGISTER("get_lua_tboolean",ring_get_lua_tboolean);
	RING_API_REGISTER("get_lua_tlightuserdata",ring_get_lua_tlightuserdata);
	RING_API_REGISTER("get_lua_tnumber",ring_get_lua_tnumber);
	RING_API_REGISTER("get_lua_tstring",ring_get_lua_tstring);
	RING_API_REGISTER("get_lua_ttable",ring_get_lua_ttable);
	RING_API_REGISTER("get_lua_tfunction",ring_get_lua_tfunction);
	RING_API_REGISTER("get_lua_tuserdata",ring_get_lua_tuserdata);
	RING_API_REGISTER("get_lua_tthread",ring_get_lua_tthread);
	RING_API_REGISTER("get_lua_numtypes",ring_get_lua_numtypes);
	RING_API_REGISTER("get_lua_minstack",ring_get_lua_minstack);
	RING_API_REGISTER("get_lua_ridx_globals",ring_get_lua_ridx_globals);
	RING_API_REGISTER("get_lua_ridx_mainthread",ring_get_lua_ridx_mainthread);
	RING_API_REGISTER("get_lua_opadd",ring_get_lua_opadd);
	RING_API_REGISTER("get_lua_opsub",ring_get_lua_opsub);
	RING_API_REGISTER("get_lua_opmul",ring_get_lua_opmul);
	RING_API_REGISTER("get_lua_opmod",ring_get_lua_opmod);
	RING_API_REGISTER("get_lua_oppow",ring_get_lua_oppow);
	RING_API_REGISTER("get_lua_opdiv",ring_get_lua_opdiv);
	RING_API_REGISTER("get_lua_opidiv",ring_get_lua_opidiv);
	RING_API_REGISTER("get_lua_opband",ring_get_lua_opband);
	RING_API_REGISTER("get_lua_opbor",ring_get_lua_opbor);
	RING_API_REGISTER("get_lua_opbxor",ring_get_lua_opbxor);
	RING_API_REGISTER("get_lua_opshl",ring_get_lua_opshl);
	RING_API_REGISTER("get_lua_opshr",ring_get_lua_opshr);
	RING_API_REGISTER("get_lua_opunm",ring_get_lua_opunm);
	RING_API_REGISTER("get_lua_opbnot",ring_get_lua_opbnot);
	RING_API_REGISTER("get_lua_opeq",ring_get_lua_opeq);
	RING_API_REGISTER("get_lua_oplt",ring_get_lua_oplt);
	RING_API_REGISTER("get_lua_ople",ring_get_lua_ople);
	RING_API_REGISTER("get_lua_gcstop",ring_get_lua_gcstop);
	RING_API_REGISTER("get_lua_gcrestart",ring_get_lua_gcrestart);
	RING_API_REGISTER("get_lua_gccollect",ring_get_lua_gccollect);
	RING_API_REGISTER("get_lua_gccount",ring_get_lua_gccount);
	RING_API_REGISTER("get_lua_gccountb",ring_get_lua_gccountb);
	RING_API_REGISTER("get_lua_gcstep",ring_get_lua_gcstep);
	RING_API_REGISTER("get_lua_gcisrunning",ring_get_lua_gcisrunning);
	RING_API_REGISTER("get_lua_gcgen",ring_get_lua_gcgen);
	RING_API_REGISTER("get_lua_gcinc",ring_get_lua_gcinc);
	RING_API_REGISTER("get_lua_gcparam",ring_get_lua_gcparam);
	RING_API_REGISTER("get_lua_hookcall",ring_get_lua_hookcall);
	RING_API_REGISTER("get_lua_hookret",ring_get_lua_hookret);
	RING_API_REGISTER("get_lua_hookline",ring_get_lua_hookline);
	RING_API_REGISTER("get_lua_hookcount",ring_get_lua_hookcount);
	RING_API_REGISTER("get_lua_hooktailcall",ring_get_lua_hooktailcall);
	RING_API_REGISTER("get_lua_maskcall",ring_get_lua_maskcall);
	RING_API_REGISTER("get_lua_maskret",ring_get_lua_maskret);
	RING_API_REGISTER("get_lua_maskline",ring_get_lua_maskline);
	RING_API_REGISTER("get_lua_maskcount",ring_get_lua_maskcount);
}
