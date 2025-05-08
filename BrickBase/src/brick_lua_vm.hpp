#ifndef BRICK_LUA_VM_HPP
#define BRICK_LUA_VM_HPP

#include <functional>

extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

/**
 * @brief Global Lua VM state pointer.
 */
extern lua_State *lua_state;

/**
 * @brief Optional callback that gets called on Lua VM exceptions.
 * @return A string describing the exception, or nullptr if none.
 */
extern std::function<const char*()> on_vm_exception_callback;

/**
 * @brief Embedded Lua module source (populated by CMake from a Lua script file).
 */
extern const char *brick_lab_lua_module;

// ---------------- Exposed Lua-C Binding Functions ----------------

/**
 * @brief Exposes `delay(ms)` function to Lua.
 *
 * @param vm_state Lua state.
 * @return Number of return values for Lua (0).
 */
int brick_lua_vm_delay(lua_State *vm_state);

/**
 * @brief Sends a command to a device from Lua using `send_command(uuid, cmd, table)`.
 *
 * @param vm_state Lua state.
 * @return Number of return values for Lua (0), or error message.
 */
int brick_lua_vm_send_command(lua_State *vm_state);

/**
 * @brief Retrieves a device handle by UUID using `get_device_from_uuid(uuid)` in Lua.
 *
 * @param vm_state Lua state.
 * @return Returns 1 value on the Lua stack (userdata or nil).
 */
int brick_lua_vm_get_device_uuid(lua_State *vm_state);

/**
 * @brief Metamethod to handle property access on `BrickDevice` Lua userdata.
 *
 * @param vm_state Lua state.
 * @return Number of return values (typically 1).
 */
int brick_device_index(lua_State *vm_state);

// ---------------- Lua VM Management ----------------

/**
 * @brief Initializes the Lua VM, registers native functions, and preloads embedded modules.
 */
void brick_lua_vm_init();

/**
 * @brief Resets the Lua VM to a clean state (optional).
 */
void brick_lua_vm_reset();

/**
 * @brief Runs a string of Lua code in the current VM.
 *
 * @param code A null-terminated Lua script.
 * @return Null on success, or a string describing the Lua error.
 */
const char* brick_lua_vm_run(const char* code);

#endif // BRICK_LUA_VM_HPP
