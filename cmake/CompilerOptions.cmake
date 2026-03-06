add_library(perdu_compiler_options INTERFACE)
add_library(perdu::compiler_options ALIAS perdu_compiler_options)

target_link_options(perdu_compiler_options INTERFACE
    -fuse-ld=lld
)

if(MSVC)
    target_compile_options(perdu_compiler_options INTERFACE
        /W4 /WX /permissive-
    )
else()
    target_compile_options(perdu_compiler_options INTERFACE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()
