cmake_minimum_required(VERSION 3.14)

project(luabus)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-fPIC -g -O0 -Wall -DLUAJIT_ENABLE_LUA52COMPAT)
add_compile_options(-fno-omit-frame-pointer)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SOURCE_SRC)

include(FetchContent)

FetchContent_Declare(
  lua
  GIT_REPOSITORY  https://github.com/maitianshouwangzhe3/lua.git
)

FetchContent_Declare(
  luna
  GIT_REPOSITORY  https://github.com/maitianshouwangzhe3/luna.git
)

FetchContent_MakeAvailable(lua)
FetchContent_MakeAvailable(luna)

add_library(luabus SHARED ${SOURCE_SRC})
set_target_properties(luabus PROPERTIES OUTPUT_NAME lbus PREFIX "")
target_include_directories(luabus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${lua_SOURCE_DIR} ${luna_SOURCE_DIR})
target_link_libraries(luabus PRIVATE lua_shared luna_shared)