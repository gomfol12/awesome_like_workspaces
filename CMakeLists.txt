cmake_minimum_required(VERSION 3.27)

project(awesome_like_workspaces
    DESCRIPTION "awesome_like_workspaces plugin for Hyprland"
    VERSION 1.0
)
set(PROJECT_AUTHOR "gomfol12")

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

file(GLOB_RECURSE SRC "*.cpp")

add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC})

target_include_directories(
  ${CMAKE_PROJECT_NAME} PRIVATE "${PROJECT_BINARY_DIR}"
                                "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
    hyprland
    libdrm
    libinput
    libudev
    pangocairo
    pixman-1
    wayland-server
    xkbcommon
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE rt PkgConfig::deps)

configure_file(include/project_vars.h.in project_vars.h @ONLY)

install(TARGETS ${CMAKE_PROJECT_NAME})
