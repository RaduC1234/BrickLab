#ifndef LUAENV_HPP
#define LUAENV_HPP

//lua
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

extern lua_State *lua_state;

// exposed functions
void brick_lua_vm_delay(uint32_t milisec);

// internal functions
void brick_lua_vm_init();
const char* brick_lua_vm_run(const char* code);

#endif //LUAENV_HPP
