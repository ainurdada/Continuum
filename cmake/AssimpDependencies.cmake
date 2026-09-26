include(FetchContent)

if(WIN32)
    set(ASSIMP_BIN_URL "https://github.com/assimp/assimp/releases/download/v6.0.5/windows-x64-v6.0.5.zip")
    set(ASSIMP_BIN_HASH "SHA256=1ab3ac83e75e64bf3fb2b67e43c3b8436b49e25a212aa0c65813415a098d7c41")
    set(LIB_NAME "assimp.lib")
    set(DLL_NAME "assimp.dll")
elseif(APPLE)
    set(ASSIMP_BIN_URL "https://github.com/assimp/assimp/releases/download/v6.0.5/macos-arm64-v6.0.5.zip")
    set(ASSIMP_BIN_HASH "SHA256=ed48e4d3d5d52a3a2f8c097060e1e5a776db832ce2733f5618abb751d0e6bd4e")
    set(LIB_NAME "libassimp.dylib")
endif()

FetchContent_Declare(
    assimp_bin
    URL "${ASSIMP_BIN_URL}"
    URL_HASH "${ASSIMP_BIN_HASH}"
)
FetchContent_MakeAvailable(assimp_bin)
FetchContent_GetProperties(assimp_bin SOURCE_DIR assimp_bin_dir)

FetchContent_Declare(
    assimp_headers_src
    URL "https://github.com/assimp/assimp/archive/refs/tags/v6.0.5.tar.gz"
    URL_HASH "SHA256=edf3749559c2b7d1f758ffb66fc5bec62186221e623b7f2e8969f17ee46ecb6f"
    SOURCE_SUBDIR "include"
)
FetchContent_MakeAvailable(assimp_headers_src)
FetchContent_GetProperties(assimp_headers_src SOURCE_DIR assimp_src_dir)

set(ASSIMP_DOUBLE_PRECISION OFF)
set(assimp_generated_include_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/assimp/include")

configure_file(
    "${assimp_src_dir}/include/assimp/config.h.in"
    "${assimp_generated_include_dir}/assimp/config.h"
    @ONLY
)

add_library(assimp::assimp SHARED IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(assimp::assimp PROPERTIES
        IMPORTED_IMPLIB "${assimp_bin_dir}/lib/${LIB_NAME}"
        IMPORTED_LOCATION "${assimp_bin_dir}/bin/${DLL_NAME}"
    )
else()
    set_target_properties(assimp::assimp PROPERTIES
        IMPORTED_LOCATION "${assimp_bin_dir}/${LIB_NAME}"
    )
endif()

set_target_properties(assimp::assimp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${assimp_src_dir}/include"
)

target_include_directories(assimp::assimp INTERFACE
    "${assimp_generated_include_dir}"
)
