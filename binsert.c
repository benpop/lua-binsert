/*
** Bisect and binary insert on sorted lists.
**
** Based on Python's bisect library.
**    Python-2.7.3/Lib/bisect.py
**    Python-2.7.3/Modules/_bisectmodule.c
**
** Authors: benpop
** Date: 2013-07-28
*/


#include "lua.h"
#include "lauxlib.h"


#if LUA_VERSION_NUM < 502
#define getn(L,i) luaL_getn(L,(i))
#define newlib(L,name,reg,nup) luaI_openlib(L,(name),(reg),(nup))
#else
#define getn(L,i) luaL_len(L,(i))
#define newlib(L,name,reg,nup) do { \
    luaL_newlibtable(L, reg); \
    lua_insert(L, -(nup)-1);  /* insert under upvalues */ \
    luaL_setfuncs(L, (reg), (nup)); \
  } while (0)
#endif


#define TINSERT   lua_upvalueindex(1)

#define T         1
#define V         2
#define CMP       3


static int cmp_lt (lua_State *L) {
#if LUA_VERSION_NUM < 502
  lua_pushboolean(L, lua_lessthan(L, 1, 2));
#else
  lua_pushboolean(L, lua_compare(L, 1, 2, LUA_OPLT));
#endif
  return 1;
}


static int bisect_body (lua_State *L) {
  int start = 1,
      mid = 1,  /* must be 1 for binsert to insert into empty table */
      state = 0,  /* insert to the left or right of the element? */
      end = (luaL_checktype(L, 1, LUA_TTABLE), getn(L, 1));
  /*
  ** 2nd arg is comparable?
  */
  int tt = lua_type(L, V);
  switch (tt) {
    case LUA_TNUMBER:  case LUA_TSTRING: {
      break;
    }
    default: {
      if (!luaL_getmetafield(L, V, "__le") &&
          !luaL_getmetafield(L, V, "__lt")) {
        luaL_argerror(L, V,
            lua_pushfstring(L, "comparable expected, got %s",
                              lua_typename(L, tt)));
        return 0;  /* never reached */
      }
    }
  }
  lua_settop(L, CMP);
  /*
  ** 3rd arg is callable?
  */
  tt = lua_type(L, CMP);
  switch (tt) {
    case LUA_TFUNCTION: {
      break;
    }
    case LUA_TNIL: {
      lua_pushcfunction(L, cmp_lt);
      lua_replace(L, CMP);
      break;
    }
    default: {
      if (!luaL_getmetafield(L, CMP, "__call")) {
        luaL_argerror(L, CMP,
            lua_pushfstring(L, "callable expected, got %s",
                              lua_typename(L, tt)));
        return 0;  /* never reached */
      }
      else lua_pop(L, 1);
    }
  }
  while (start <= end) {
    mid = (start + end) / 2;
    lua_pushvalue(L, CMP);  /* compare function */
    lua_pushvalue(L, V);  /* value to insert */
    lua_rawgeti(L, T, mid);  /* T[mid] */
    lua_call(L, 2, 1);  /* compare(value, T[mid]) */
    if (lua_toboolean(L, -1)) {
      end   = mid - 1; state = 0;
    }
    else {
      start = mid + 1; state = 1;
    }
    lua_pop(L, 1);
  }
  lua_pushinteger(L, (mid + state));
  return 1;
}


static int bisect (lua_State *L) {
  return bisect_body(L);
}


static int binsert (lua_State *L) {
  bisect_body(L);
  lua_pushvalue(L, TINSERT);
  lua_pushvalue(L, T);
  lua_pushvalue(L, -3);  /* copy index */
  lua_pushvalue(L, V);
  lua_call(L, 3, 0);  /* table.insert(t, (mid+state), value) */
  return 1;  /* index */
}


static void push_table_insert (lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  luaL_checktype(L, -1, LUA_TTABLE);
  lua_getfield(L, -1, "table");
  luaL_checktype(L, -1, LUA_TTABLE);
  lua_getfield(L, -1, "insert");
  luaL_checktype(L, -1, LUA_TFUNCTION);
}


static const luaL_Reg funcs[] = {
  {"bisect", bisect},
  {"binsert", binsert},
  {NULL, NULL}
};


int luaopen_binsert (lua_State *L) {
  push_table_insert(L);
  newlib(L, "binsert", funcs, 1);
  return 1;
}

