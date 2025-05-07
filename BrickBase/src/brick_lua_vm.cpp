#include "brick_lua_vm.hpp"

#include <freertos/FreeRTOS.h>

lua_State *vm_state = nullptr;

void brick_lua_vm_delay(uint32_t mili) {
    vTaskDelay(mili);
}

void brick_lua_vm_init() {
    vm_state = luaL_newstate();
    luaL_openlibs(vm_state);
}

const char *brick_lua_vm_run(const char *code) {
    assert(vm_state && "Lua VM not initialized");

    int status = luaL_dostring(vm_state, code);
    if (status != LUA_OK) {
        const char *error_msg = lua_tostring(vm_state, -1);
        lua_pop(vm_state, 1);
        return error_msg ? error_msg : "Unknown Lua error";
    }

    return "Success";
}
