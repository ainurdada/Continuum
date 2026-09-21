include(../../cmake/LLVMDependencies.cmake)

add_executable(
    ContinuumReflectionGenerator

    main.cpp
    emitter.cpp
)

target_include_directories(
    ContinuumReflectionGenerator
    SYSTEM PRIVATE
    "${llvm_SOURCE_DIR}/llvm/include"
    "${llvm_BINARY_DIR}/include"
    "${llvm_SOURCE_DIR}/clang/include"
    "${llvm_BINARY_DIR}/tools/clang/include"
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

add_dependencies(ContinuumReflectionGenerator clang-resource-headers)

get_target_property(clangResourceIncludeDir clang-resource-headers RUNTIME_OUTPUT_DIRECTORY)

cmake_path(GET clangResourceIncludeDir PARENT_PATH clangResourceDir)

target_compile_definitions(ContinuumReflectionGenerator PRIVATE "CONTINUUM_CLANG_RESOURCE_DIR=\"${clangResourceDir}\"")

target_compile_options(ContinuumReflectionGenerator PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
