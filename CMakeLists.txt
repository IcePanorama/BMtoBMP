cmake_minimum_required (VERSION 3.28)

project(BMtoBMP C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(WIN32_CC i686-w64-mingw32-gcc)
set(WIN64_CC x86_64-w64-mingw32-gcc)

add_compile_options(
    -std=c99
    -Wpedantic
    -Wextra
    -Werror
    -Wall
    -Wstrict-aliasing=3
    -Wwrite-strings
    -Wvla
    -Wcast-align=strict
    -Wstrict-prototypes
    -Wstringop-overflow=4
    -Wshadow
    -fanalyzer
    -DBMtoBMP_DEBUG_OUTPUT
)

set(AUTO_FMT clang-format)
set(CODE_STYLE GNU)

set(INCL_DIR "${PROJECT_SOURCE_DIR}/include")
set(INCL_FILES
    "${INCL_DIR}/bm_to_bmp_converter.h"
)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(SRC_FILES
    "${SRC_DIR}/main.c"
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

# Format target
add_custom_target(format
    COMMAND ${AUTO_FMT} -style=${CODE_STYLE} -i ${SRC_FILES} ${INCL_FILES}
    COMMENT "Auto-formatting code."
)

# Test target
add_custom_target(test
    COMMAND valgrind --leak-check=yes ./${PROJECT_NAME} AUTOGRPH.BM AUTOGRPH.PAL
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Testing for memory leaks with Valgrind."
)

# `full` target
add_custom_target(full
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test
    COMMENT "Running full build (clean, format, build, and test)."
)

# Default/All target
add_executable(${PROJECT_NAME} ${SRC_FILES})
add_dependencies(${PROJECT_NAME} format)
target_include_directories(${PROJECT_NAME} PRIVATE ${INCL_DIR})

# Win32 target
add_custom_target(win32
    COMMAND ${WIN32_CC} ${CMAKE_C_FLAGS} -o "${PROJECT_NAME}_i686.exe" ${SRC_FILES} -I${INCL_DIR}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Building 32-bit executable."
)

# Win64 target
add_custom_target(win64
    COMMAND ${WIN64_CC} ${CMAKE_C_FLAGS} -o "${PROJECT_NAME}_x86_64.exe" ${SRC_FILES} -I${INCL_DIR}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Building 64-bit executable."
)

# Release target
add_custom_target(release
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target win32
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target win64
    COMMENT "Running full build (clean, format, build, and test)."
)

# Clean target
set(extra_files_to_clean
    "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}_i686.exe"
    "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}_x86_64.exe"
)
set_property(
	TARGET ${PROJECT_NAME}
	APPEND
	PROPERTY ADDITIONAL_CLEAN_FILES ${extra_files_to_clean}
)
