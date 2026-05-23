cmake_minimum_required(VERSION 3.14)

project(luabus)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-fPIC -g -O0 -Wall -DLUAJIT_ENABLE_LUA52COMPAT)
add_compile_options(-fno-omit-frame-pointer)

set(LUA_DIR ${PROJECT_SOURCE_DIR}/3rd/lua)
set(LUNA_DIR ${PROJECT_SOURCE_DIR}/3rd/luna)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SOURCE_SRC)

add_library(luabus SHARED ${SOURCE_SRC})
set_target_properties(luabus PROPERTIES OUTPUT_NAME lbus PREFIX "")
target_include_directories(luabus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LUA_DIR} ${LUNA_DIR})