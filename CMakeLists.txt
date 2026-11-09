cmake_minimum_required(VERSION 3.22.1)
project(gemini-cli LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/font
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/internal/ImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/internal/ImGui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/internal/Android_draw
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/internal/Android_touch
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/internal/native_surface
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/draw/ImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/draw/ImGui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/protect
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/vendor/minitouch/jni/vendor/libevdev/include
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/vendor/minitouch/jni/vendor/libevdev/source/libevdev
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/png
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/png/images
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/func
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game
    ${CMAKE_CURRENT_SOURCE_DIR}/src/other
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protect
)

file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protect/license.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/draw/*.cpp
)

list(FILTER SRC_FILES EXCLUDE REGEX "imgui_demo\\.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "imgui_impl_vulkan\\.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "stb_image\\.cpp")

add_executable(gemini-cli ${SRC_FILES})

target_link_libraries(gemini-cli
    PRIVATE
    log
    android
    EGL
    GLESv3
    z
    dl
)

target_link_options(gemini-cli PRIVATE
    -Wl,--gc-sections
    -Wl,--strip-all
    -pie
    -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,noexecstack
    -Wl,-z,max-page-size=16384
)
