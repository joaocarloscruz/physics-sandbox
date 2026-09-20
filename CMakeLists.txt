cmake_minimum_required(VERSION 3.24)
project(PhysicsSandbox VERSION 2.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
include(FetchContent)

# Pin the engine; compile its unchanged sources privately for a portable binary.
FetchContent_Declare(physics_engine
    GIT_REPOSITORY https://github.com/joaocarloscruz/2d-physicsengine.git
    GIT_TAG ed169e7caa2c58d717ad63b0950775a3de6732bd
    SOURCE_SUBDIR sandbox_dependency_only)
# A deliberately absent source subdirectory fetches without adding upstream's
# shared-library/demo/test targets to this build.
FetchContent_MakeAvailable(physics_engine)
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${physics_engine_SOURCE_DIR}/src/physics/*.cpp")
add_library(sandbox_engine STATIC ${ENGINE_SOURCES})
target_include_directories(sandbox_engine PUBLIC "${physics_engine_SOURCE_DIR}/include")
target_compile_definitions(sandbox_engine PUBLIC _USE_MATH_DEFINES)
add_library(sandbox_model sandbox/scene.cpp)
target_include_directories(sandbox_model PUBLIC sandbox)
target_link_libraries(sandbox_model PUBLIC sandbox_engine)

option(SANDBOX_BUILD_APP "Build the graphical sandbox" ON)
if(SANDBOX_BUILD_APP)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8)
    FetchContent_MakeAvailable(raylib)
    add_executable(Sandbox sandbox/main.cpp sandbox/ui.cpp)
    target_link_libraries(Sandbox PRIVATE sandbox_model raylib)
    if(MINGW)
        target_link_options(Sandbox PRIVATE -static -mwindows)
    elseif(MSVC)
        target_link_options(Sandbox PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    endif()
    add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:Sandbox>/assets")
    install(TARGETS Sandbox RUNTIME DESTINATION . COMPONENT Runtime)
    install(DIRECTORY assets DESTINATION . COMPONENT Runtime)
    install(DIRECTORY docs DESTINATION . COMPONENT Runtime)
    install(FILES README.md LICENSE THIRD_PARTY.md DESTINATION . COMPONENT Runtime)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(sandbox_tests tests/scene_tests.cpp)
    target_link_libraries(sandbox_tests PRIVATE sandbox_model)
    if(MINGW)
        target_link_options(sandbox_tests PRIVATE -static)
    endif()
    add_test(NAME sandbox_integration COMMAND sandbox_tests)
endif()
