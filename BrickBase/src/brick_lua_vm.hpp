#ifndef LUAENV_HPP
#define LUAENV_HPP

//lua
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

extern lua_State *lua_state;

extern const char *brick_lab_lua_module; // defined in cmake

// exposed functions
int brick_lua_vm_delay(lua_State *vm_state);
int brick_lua_vm_send_command(lua_State *vm_state);
int brick_lua_vm_get_device_uuid(lua_State *vm_state);

// internal functions
void brick_lua_vm_init();
void brick_lua_vm_reset();

const char* brick_lua_vm_run(const char* code);

#endif //LUAENV_HPP
