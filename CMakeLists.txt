cmake_minimum_required(VERSION 3.10)
project(ALink C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add source files
file(GLOB SOURCES src/*.c)

add_executable(ALink ${SOURCES})

if(MSVC)
	target_compile_options(ALink PRIVATE -D_CRT_SECURE_NO_WARNINGS)
endif()


# Optional: Set output directory
set_target_properties(ALink PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
