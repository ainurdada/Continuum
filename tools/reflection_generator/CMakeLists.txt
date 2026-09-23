include(../../cmake/LLVMDependencies.cmake)

add_executable(
    ContinuumReflectionGenerator

    main.cpp
    private/emitter.cpp
    private/analysis/type_analyzer.cpp
    private/analysis/modifier_analyzer.cpp
    private/analysis/markers.cpp
)

target_include_directories(
    ContinuumReflectionGenerator
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/private"
)

target_include_directories(
    ContinuumReflectionGenerator
    SYSTEM PRIVATE
    "${LLVM_INCLUDE_DIRS}"
    "${CLANG_INCLUDE_DIRS}"
)

target_compile_definitions(
    ContinuumReflectionGenerator
    PRIVATE
    __STDC_CONSTANT_MACROS
    __STDC_FORMAT_MACROS
    __STDC_LIMIT_MACROS
)

target_link_libraries(
    ContinuumReflectionGenerator
    PRIVATE
    clangTooling
)

set(clangResourceDir "${CONTINUUM_LLVM_SDK_PATH}/lib/clang/22")

target_compile_definitions(ContinuumReflectionGenerator PRIVATE "CONTINUUM_CLANG_RESOURCE_DIR=\"${clangResourceDir}\"")

target_compile_options(ContinuumReflectionGenerator PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(ContinuumReflectionGenerator PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang>:-fno-rtti>
    $<$<CXX_COMPILER_ID:MSVC>:/GR->
)
endif()


if(APPLE)
    target_link_options(ContinuumReflectionGenerator PRIVATE
        "LINKER:-lto_library,${CONTINUUM_LLVM_SDK_PATH}/lib/libLTO.dylib"
    )
endif()

