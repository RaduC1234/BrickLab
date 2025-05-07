# Automatically include all source files in src/
file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# === Embed Lua Script ===
set(LUA_SOURCE ${CMAKE_SOURCE_DIR}/scripts/brick_lab.lua)
set(LUA_C_OUTPUT ${CMAKE_BINARY_DIR}/brick_lab_lua.c)

file(READ ${LUA_SOURCE} LUA_CONTENT)
string(REPLACE "\\" "\\\\" LUA_CONTENT "${LUA_CONTENT}")
string(REPLACE "\"" "\\\"" LUA_CONTENT "${LUA_CONTENT}")
string(REPLACE "\n" "\\n\"\n\"" LUA_CONTENT "${LUA_CONTENT}")

file(WRITE ${LUA_C_OUTPUT}
        "// Auto-generated from brick_lab.lua\n"
        "const char *brick_lab_lua_module = \"${LUA_CONTENT}\";\n"
)

# Add the generated file to the source list
list(APPEND app_sources ${LUA_C_OUTPUT})

# === Register as an ESP-IDF component ===
idf_component_register(SRCS ${app_sources})
