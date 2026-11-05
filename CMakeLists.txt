cmake_minimum_required(VERSION 3.21)

project(MiniCAE_Solvers VERSION 0.1.0 LANGUAGES CXX)

add_executable(minicae_demo src/main.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MINICAE_PLATFORM "macOS")
else()
    set(MINICAE_PLATFORM "${CMAKE_SYSTEM_NAME}")
endif()
set(MINICAE_CONFIGURATION "$<IF:$<BOOL:$<CONFIG>>,$<CONFIG>,Unspecified>")
target_compile_definitions(minicae_demo PRIVATE
    MINICAE_PLATFORM="${MINICAE_PLATFORM}"
    MINICAE_CONFIGURATION="${MINICAE_CONFIGURATION}"
)

set_target_properties(minicae_demo PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(minicae_demo PRIVATE /W4 /utf-8 /EHsc)
else()
    target_compile_options(minicae_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME minicae_startup COMMAND minicae_demo)
    set_tests_properties(minicae_startup PROPERTIES
        PASS_REGULAR_EXPRESSION "MiniCAE-Solvers [(]${MINICAE_PLATFORM} ${MINICAE_CONFIGURATION}[)]: C[+][+]17 startup OK"
    )
endif()
