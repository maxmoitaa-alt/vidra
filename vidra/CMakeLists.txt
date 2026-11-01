cmake_minimum_required(VERSION 3.28)
project(vidra VERSION 1.0.2 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/CompilerOptions.cmake)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
enable_testing()
include(FetchContent)
# Luau 0.653 still declares CMake compatibility below 3.5.
# CMake 4.x removed that compatibility unless a policy floor is supplied.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()
FetchContent_Declare(luau GIT_REPOSITORY https://github.com/luau-lang/luau.git GIT_TAG 0.653)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(luau)
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11 xext xfixes xi)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(GL REQUIRED gl)
find_package(Vulkan QUIET)

add_custom_target(tokens
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/tokens_gen.py
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_library(vidra_core STATIC
  src/core/log.cpp src/core/paths.cpp src/core/json.cpp src/core/settings.cpp
  src/color/hct.cpp src/color/palette.cpp src/gfx/sdf.cpp src/gfx/text.cpp
  src/gfx/gl_backend.cpp src/gfx/renderer_factory.cpp src/platform/x11_window.cpp src/runtime/vm.cpp
  src/ui/widget.cpp src/ui/widgets.cpp src/ui/editor.cpp src/ui/screens.cpp src/ui/root.cpp)
add_dependencies(vidra_core tokens)
target_include_directories(vidra_core PUBLIC
  include ${X11_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS}
  ${HARFBUZZ_INCLUDE_DIRS} ${GL_INCLUDE_DIRS})
target_link_libraries(vidra_core PUBLIC
  ${X11_LIBRARIES} ${FREETYPE_LIBRARIES} ${HARFBUZZ_LIBRARIES} ${GL_LIBRARIES}
  Luau.VM Luau.Compiler Luau.Ast)
if(Vulkan_FOUND)
  target_sources(vidra_core PRIVATE src/gfx/vk_backend.cpp)
  target_link_libraries(vidra_core PUBLIC Vulkan::Vulkan)
  target_compile_definitions(vidra_core PUBLIC VIDRA_HAVE_VULKAN)
endif()

add_executable(vidra src/app/main.cpp src/app/app.cpp)
target_link_libraries(vidra PRIVATE vidra_core)
add_dependencies(vidra tokens)

add_custom_target(shaders ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders)
add_dependencies(vidra shaders)

foreach(t color spring sdf json runtime editor window renderer settings)
  add_executable(test_${t} tests/test_${t}.cpp)
  target_link_libraries(test_${t} PRIVATE vidra_core)
  add_test(NAME ${t} COMMAND test_${t})
  set_tests_properties(${t} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()

install(TARGETS vidra RUNTIME DESTINATION bin)
