cmake_minimum_required(VERSION 3.11)
project(PhysicsSandbox VERSION 1.0)

# Include FetchContent module to fetch dependencies from git
include(FetchContent)

# Add raylib from subdirectory
add_subdirectory(libs/raylib)

# Declare the PhysicsEngine dependency from GitHub
FetchContent_Declare(
    PhysicsEngine
    GIT_REPOSITORY https://github.com/joaocarloscruz/2d-physicsengine.git
    GIT_TAG main
)

# Make the dependency available to the project
FetchContent_MakeAvailable(PhysicsEngine)

# Add the main executable
add_executable(Sandbox sandbox/main.cpp)

target_include_directories(Sandbox PRIVATE ${PhysicsEngine_SOURCE_DIR}/include)

# Link the executable against raylib and the PhysicsEngine
# FetchContent will automatically handle the include directories for PhysicsEngine
target_link_libraries(Sandbox PRIVATE raylib PhysicsEngine)

add_custom_command(
  TARGET Sandbox POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          $<TARGET_FILE:PhysicsEngine>
          $<TARGET_FILE_DIR:Sandbox>
)
