cmake_minimum_required(VERSION 3.2)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project(Engine2)
find_package(OpenGL REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${Engine2_SOURCE_DIR})
if(NOT EXISTS ${Engine2_SOURCE_DIR}/lib/glfw/build)
    execute_process(
        COMMAND cmake -S ${Engine2_SOURCE_DIR}/lib/glfw -B ${Engine2_SOURCE_DIR}/lib/glfw/build
        OUTPUT_VARIABLE GLFW_BUILD
    )
    execute_process(
        COMMAND make
        OUTPUT_VARIABLE GLFW_MAKE
     	WORKING_DIRECTORY ${Engine2_SOURCE_DIR}/lib/glfw/build
     )
    message("BUILT GLFW")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -stdlib=libc++")
endif()

file(GLOB SOURCES "src/*.cpp" "src/*.c" "src/*/*.cpp" "src/*/*.c" "ext/*.c" "ext/*.cpp" "ext/*/*.cpp" "ext/*/*.c")
add_executable(Engine2 ${SOURCES})

target_include_directories(Engine2 PUBLIC ${Engine2_SOURCE_DIR}/include ${Engine2_SOURCE_DIR}/src)


target_link_libraries(Engine2 ${Engine2_SOURCE_DIR}/lib/glfw/build/src/libglfw3.a OpenGL::GL m X11 pthread Xrandr dl)
