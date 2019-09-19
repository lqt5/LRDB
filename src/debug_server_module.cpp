#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#include <iostream>

#include "lrdb/server.hpp"

#if !defined(WIN32) && !defined(_WIN32)
#define stricmp strcasecmp
#else
#define stricmp _stricmp
#endif

typedef std::unique_ptr<lrdb::server> server_ptr;

int lrdb_activate(lua_State* L) {
  server_ptr* server = (server_ptr*)lua_touserdata(L, lua_upvalueindex(1));
  if (lua_isnumber(L, 1)) {
    server->reset(new lrdb::server((int16_t)lua_tonumber(L, 1)));
  } else {
    server->reset(new lrdb::server(21110));
  }
  (*server)->reset(L);
  return 0;
}

int lrdb_deactivate(lua_State* L) {
  server_ptr* server = (server_ptr*)lua_touserdata(L, lua_upvalueindex(1));
  server->reset();
  return 0;
}

int lrdb_enable(lua_State *L) {
	server_ptr* server = (server_ptr*)lua_touserdata(L, lua_upvalueindex(1));
	if (server->get() == nullptr)
		return 0;
	luaL_checktype(L, 1, LUA_TBOOLEAN);
	bool enabled = lua_toboolean(L, 1) == 1;
	server->get()->reset(enabled ? L : NULL, false);
	return 0;
}

int lrdb_step(lua_State *L) {
	server_ptr* server = (server_ptr*)lua_touserdata(L, lua_upvalueindex(1));
	if (server->get() == nullptr)
		return 0;
	const char *mode = luaL_optstring(L, 1, "in");
	lrdb::debugger& debugger = server->get()->get_debugger();
	if (stricmp(mode, "in") == 0) {
		debugger.step_in();
	}
	else if (stricmp(mode, "out") == 0) {
		debugger.step_out();
	}
	else if (stricmp(mode, "over") == 0) {
		debugger.step_over();
	}
	return 0;
}

int lrdb_poll(lua_State *L) {
	server_ptr* server = (server_ptr*)lua_touserdata(L, lua_upvalueindex(1));
	if (server->get() == nullptr)
		return 0;
	lrdb::debugger& debugger = server->get()->get_debugger();
	debugger.poll();

	return 0;
}

int lrdb_destruct(lua_State* L) {
  server_ptr* server = (server_ptr*)lua_touserdata(L, 1);
  server->~server_ptr();
  return 0;
}

#if defined(_WIN32) || defined(_WIN64)
extern "C" __declspec(dllexport)
#else
extern "C" __attribute__((visibility("default")))
#endif
    int luaopen_lrdb_server(lua_State* L) {
  //	luaL_dostring(L, "debug=nil");
  lua_createtable(L, 0, 3);
  int mod = lua_gettop(L);

  void* storage = lua_newuserdata(L, sizeof(server_ptr));
  new (storage) server_ptr();
  int sserver = lua_gettop(L);
  lua_createtable(L, 0, 1);
  lua_pushcclosure(L, &lrdb_destruct, 0);
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, sserver);

  lua_pushvalue(L, sserver);
  lua_pushcclosure(L, &lrdb_activate, 1);
  lua_setfield(L, mod, "activate");
  lua_pushvalue(L, sserver);
  lua_pushcclosure(L, &lrdb_deactivate, 1);
  lua_setfield(L, mod, "deactivate");
  lua_pushvalue(L, sserver);
  lua_pushcclosure(L, &lrdb_enable, 1);
  lua_setfield(L, mod, "enable");
  lua_pushvalue(L, sserver);
  lua_pushcclosure(L, &lrdb_step, 1);
  lua_setfield(L, mod, "step");
  lua_pushcclosure(L, &lrdb_poll, 1);
  lua_setfield(L, mod, "poll");

  lua_pushvalue(L, mod);
  return 1;
}