cmake_minimum_required(VERSION 3.14)

project(luabus)

add_compile_options(-fPIC -g -O0 -Wall -DLUAJIT_ENABLE_LUA52COMPAT)
add_compile_options(-fno-omit-frame-pointer)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SOURCE_SRC)

add_subdirectory(3rd/lua)
add_subdirectory(3rd/luna)

add_library(luabus SHARED ${SOURCE_SRC})
set_target_properties(luna_static PROPERTIES OUTPUT_NAME luabus PREFIX "")
target_include_directories(luabus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/3rd/luna ${CMAKE_CURRENT_SOURCE_DIR}/3rd/lua)
target_link_libraries(luabus PRIVATE lua_static luna_static)    