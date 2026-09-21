cmake_minimum_required(VERSION 4.3)

project(
    Continuum
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)
include(cmake/ModuleHelper.cmake)
include(cmake/ContinuumRuntime.cmake)
include(cmake/ContinuumReflection.cmake)

add_subdirectory(engine)
add_subdirectory(tools/reflection_generator)

add_subdirectory(projects/demo)
