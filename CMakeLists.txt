cmake_minimum_required(VERSION 3.10)
project(ALink C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES 
    src/alink.c
    src/alink.h
    src/coff.c
    src/cofflib.c
    src/combine.c
    src/objload.c
    src/output.c
    src/output_com.c
    src/output_exe.c
    src/output_pe.c
    src/util.c
)

add_executable(ALink ${SOURCES})

if(MSVC)
	target_compile_options(ALink PRIVATE -D_CRT_SECURE_NO_WARNINGS)
endif()


# Optional: Set output directory
set_target_properties(ALink PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
