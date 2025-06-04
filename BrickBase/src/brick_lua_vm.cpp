#include "brick_lua_vm.hpp"

#include <brick_i2c_api.h>
#include <brick_i2c_host.hpp>
#include <freertos/FreeRTOS.h>

#include <esp_log.h>

lua_State *vm_state = nullptr;
std::function<const char*()> on_vm_exception_callback = nullptr;

int brick_lua_vm_delay(lua_State *L) {
    int ms = luaL_checkinteger(L, 1);
    vTaskDelay(pdMS_TO_TICKS(ms));
    return 0;
}

int brick_lua_vm_send_command(lua_State *vm_state) {
    const char *uuid_str = luaL_checkstring(vm_state, 1);
    int cmd_type = luaL_checkinteger(vm_state, 2);
    luaL_checktype(vm_state, 3, LUA_TTABLE);

    // --- Validate UUID string length ---
    if (!uuid_str || strlen(uuid_str) != 36) {
        return luaL_error(vm_state, "UUID must be exactly 36 characters long");
    }

    // --- Convert UUID string to brick_uuid_t safely ---
    brick_uuid_t uuid;
    int byte_index = 0;

    for (int i = 0; i < 36 && byte_index < 16;) {
        if (uuid_str[i] == '-') {
            i++;
            continue;
        }

        if (i + 1 >= 36) {
            return luaL_error(vm_state, "Malformed UUID string: unexpected end at position %d", i);
        }

        auto hex_char_to_int = [](char c) -> int {
            if ('0' <= c && c <= '9') return c - '0';
            if ('a' <= c && c <= 'f') return c - 'a' + 10;
            if ('A' <= c && c <= 'F') return c - 'A' + 10;
            return -1;
        };

        int high = hex_char_to_int(uuid_str[i]);
        int low = hex_char_to_int(uuid_str[i + 1]);

        if (high < 0 || low < 0) {
            return luaL_error(vm_state, "Malformed UUID string: non-hex characters at position %d", i);
        }

        uuid.bytes[byte_index++] = static_cast<uint8_t>((high << 4) | low);
        i += 2;
    }

    if (byte_index != 16) {
        return luaL_error(vm_state, "Incomplete UUID: expected 16 bytes, got %d", byte_index);
    }

    // --- Lookup device by UUID ---
    brick_device_t *dev = brick_i2c_get_device_uuid(uuid);
    if (!dev) return luaL_error(vm_state, "Device not found for given UUID");

    // --- Handle supported command ---
    if (cmd_type == CMD_LED_RGB && dev->device_type == LED_RGB) {
        lua_getfield(vm_state, 3, "red");
        lua_getfield(vm_state, 3, "green");
        lua_getfield(vm_state, 3, "blue");

        if (!lua_isinteger(vm_state, -3) || !lua_isinteger(vm_state, -2) || !lua_isinteger(vm_state, -1)) {
            lua_pop(vm_state, 3);
            return luaL_error(vm_state, "RGB values must be integers");
        }

        dev->impl.led_rgb.red = lua_tointeger(vm_state, -3);
        dev->impl.led_rgb.green = lua_tointeger(vm_state, -2);
        dev->impl.led_rgb.blue = lua_tointeger(vm_state, -1);
        lua_pop(vm_state, 3);
    } else {
        return luaL_error(vm_state, "Unsupported command or mismatched device type");
    }

    // --- Create and send command ---
    const brick_command_t cmd = {
        .command = static_cast<brick_command_type_t>(cmd_type),
        .device = dev
    };

    brick_i2c_send_device_command(&cmd);
    return 0;
}

int brick_lua_vm_get_device_uuid(lua_State *vm_state) {
    const char *uuid_str = luaL_checkstring(vm_state, 1);

    brick_device_t *dev = brick_i2c_get_device_uuid(uuid_str);
    if (!dev) {
        lua_pushnil(vm_state);
        return 1;
    }

    brick_device_t **ud = static_cast<brick_device_t **>(lua_newuserdata(vm_state, sizeof(brick_device_t *)));
    *ud = dev;

    // set metatable for the object
    luaL_getmetatable(vm_state, "BrickDevice");
    lua_setmetatable(vm_state, -2);

    return 1;
}

int brick_device_index(lua_State *vm_state) {
    auto **ud = static_cast<brick_device_t **>(luaL_checkudata(vm_state, 1, "BrickDevice"));
    const char *key = luaL_checkstring(vm_state, 2);

    if (strcmp(key, "device_type") == 0) {
        lua_pushinteger(vm_state, (*ud)->device_type);
        return 1;
    }

    lua_pushnil(vm_state);
    return 1;
}

void brick_lua_vm_init() {
    vm_state = luaL_newstate();
    luaL_openlibs(vm_state); // Load standard Lua libraries

    // --- Register global C functions (into _G) ---
    static constexpr luaL_Reg global_funcs[] = {
        {"delay", brick_lua_vm_delay},
        {nullptr, nullptr}
    };
    lua_getglobal(vm_state, "_G");
    luaL_setfuncs(vm_state, global_funcs, 0);
    lua_pop(vm_state, 1);

    // --- Register 'brick' namespace ---
    static constexpr luaL_Reg brick_funcs[] = {
        {"get_device_from_uuid", brick_lua_vm_get_device_uuid},
        {"send_command", brick_lua_vm_send_command},
        {nullptr, nullptr}
    };
    luaL_newlib(vm_state, brick_funcs); // stack: [brick table]

    // Add constants to 'brick'
    lua_pushinteger(vm_state, CMD_IDENTIFY);
    lua_setfield(vm_state, -2, "CMD_IDENTIFY");
    lua_pushinteger(vm_state, CMD_LED);
    lua_setfield(vm_state, -2, "CMD_LED");
    lua_pushinteger(vm_state, CMD_LED_DOUBLE);
    lua_setfield(vm_state, -2, "CMD_LED_DOUBLE");
    lua_pushinteger(vm_state, CMD_LED_RGB);
    lua_setfield(vm_state, -2, "CMD_LED_RGB");
    lua_pushinteger(vm_state, CMD_SERVO_SET_ANGLE);
    lua_setfield(vm_state, -2, "CMD_SERVO_SET_ANGLE");
    lua_pushinteger(vm_state, CMD_STEPPER_MOVE);
    lua_setfield(vm_state, -2, "CMD_STEPPER_MOVE");
    lua_pushinteger(vm_state, CMD_SENSOR_GET_CM);
    lua_setfield(vm_state, -2, "CMD_SENSOR_GET_CM");

    // === Device types ===
    lua_pushinteger(vm_state, LED_RGB);
    lua_setfield(vm_state, -2, "DEVICE_LED_RGB");
    lua_pushinteger(vm_state, LED_SINGLE);
    lua_setfield(vm_state, -2, "DEVICE_LED_SINGLE");

    // Finalize 'brick' global table
    lua_setglobal(vm_state, "brick"); // _G["brick"] = brick table

    // --- Set BrickDevice metatable ---
    luaL_newmetatable(vm_state, "BrickDevice");
    lua_pushcfunction(vm_state, brick_device_index);
    lua_setfield(vm_state, -2, "__index");
    lua_pop(vm_state, 1); // pop metatable

    // --- Preload embedded Lua module as 'brick_lab' ---
    lua_getglobal(vm_state, "package");
    lua_getfield(vm_state, -1, "preload");

    lua_pushcfunction(vm_state, [](lua_State *L) -> int {
                      luaL_dostring(L, brick_lab_lua_module); // Execute embedded Lua string
                      return 1; // Return value becomes module result for `require`
                      });

    lua_setfield(vm_state, -2, "brick_lab"); // package.preload["brick_lab"] = loader
    lua_pop(vm_state, 2); // pop preload and package
}

void brick_lua_vm_reset() {
    if (vm_state) {
        lua_close(vm_state);
        vm_state = nullptr;
    }
    brick_lua_vm_init();
}

const char *brick_lua_vm_run(const char *code) {
    brick_lua_vm_reset();
    assert(vm_state && "Lua VM not initialized");

    printf("Lua code:\n%s\n", code);

    // Load the Lua code
    if (luaL_loadstring(vm_state, code) != LUA_OK) {
        const char *err = lua_tostring(vm_state, -1);
        lua_pop(vm_state, 1);
        return err;
    }

    // Run it protected
    if (lua_pcall(vm_state, 0, LUA_MULTRET, 0) != LUA_OK) {
        const char *err = lua_tostring(vm_state, -1);
        lua_pop(vm_state, 1);
        return err;
    }

    return nullptr; // success
}
