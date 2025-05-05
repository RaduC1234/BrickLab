include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(BrickModulePic_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(BrickModulePic_default_default_XC8_FILE_TYPE_assemble)
add_library(BrickModulePic_default_default_XC8_assemble OBJECT ${BrickModulePic_default_default_XC8_FILE_TYPE_assemble})
    BrickModulePic_default_default_XC8_assemble_rule(BrickModulePic_default_default_XC8_assemble)
    list(APPEND BrickModulePic_default_library_list "$<TARGET_OBJECTS:BrickModulePic_default_default_XC8_assemble>")
endif()

# Handle files with suffix S, for group default-XC8
if(BrickModulePic_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(BrickModulePic_default_default_XC8_assemblePreprocess OBJECT ${BrickModulePic_default_default_XC8_FILE_TYPE_assemblePreprocess})
    BrickModulePic_default_default_XC8_assemblePreprocess_rule(BrickModulePic_default_default_XC8_assemblePreprocess)
    list(APPEND BrickModulePic_default_library_list "$<TARGET_OBJECTS:BrickModulePic_default_default_XC8_assemblePreprocess>")
endif()

# Handle files with suffix [cC], for group default-XC8
if(BrickModulePic_default_default_XC8_FILE_TYPE_compile)
add_library(BrickModulePic_default_default_XC8_compile OBJECT ${BrickModulePic_default_default_XC8_FILE_TYPE_compile})
    BrickModulePic_default_default_XC8_compile_rule(BrickModulePic_default_default_XC8_compile)
    list(APPEND BrickModulePic_default_library_list "$<TARGET_OBJECTS:BrickModulePic_default_default_XC8_compile>")
endif()

add_executable(${BrickModulePic_default_image_name} ${BrickModulePic_default_library_list})

target_link_libraries(${BrickModulePic_default_image_name} PRIVATE ${BrickModulePic_default_default_XC8_FILE_TYPE_link})

# Add the link options from the rule file.
BrickModulePic_default_link_rule(${BrickModulePic_default_image_name})


# Post build target to copy built file to the output directory.
add_custom_command(TARGET ${BrickModulePic_default_image_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${BrickModulePic_default_output_dir}
                    COMMAND ${CMAKE_COMMAND} -E copy ${BrickModulePic_default_image_name} ${BrickModulePic_default_output_dir}/${BrickModulePic_default_original_image_name}
                    BYPRODUCTS ${BrickModulePic_default_output_dir}/${BrickModulePic_default_original_image_name})
