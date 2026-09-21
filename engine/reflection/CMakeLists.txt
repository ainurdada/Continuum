add_library(
    ContinuumEngine_Reflection
    INTERFACE
)

set_module_defaults(ContinuumEngine_Reflection INTERFACE)

target_compile_options(ContinuumEngine_Reflection INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
)