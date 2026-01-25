cmake_minimum_required(VERSION 3.14)

project(luabus)

add_compile_options(-fPIC -g -O0 -Wall -DLUAJIT_ENABLE_LUA52COMPAT)
add_compile_options(-fno-omit-frame-pointer)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SOURCE_SRC)

include(FetchContent)

FetchContent_Declare(
  lua
  GIT_REPOSITORY  https://github.com/maitianshouwangzhe3/lua.git
)

FetchContent_MakeAvailable(lua)

add_library(luabus SHARED ${SOURCE_SRC})
set_target_properties(luabus PROPERTIES OUTPUT_NAME lbus PREFIX "")
target_include_directories(luabus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${lua_SOURCE_DIR})
target_link_libraries(luabus PRIVATE lua_static)