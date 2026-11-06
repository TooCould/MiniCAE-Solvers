cmake_minimum_required(VERSION 3.21)

project(MiniCAE_Solvers VERSION 0.1.0 LANGUAGES CXX)

add_executable(minicae_demo src/main.cpp "src/linalg/vector.cpp" "src/linalg/dense_matrix.cpp")
target_include_directories(minicae_demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

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
    add_executable(export_dense_matrix tests/linalg/export_dense_matrix.cpp
        src/linalg/dense_matrix.cpp src/linalg/vector.cpp)
    target_include_directories(export_dense_matrix PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(export_dense_matrix PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    if(MSVC)
        target_compile_options(export_dense_matrix PRIVATE /W4 /utf-8 /EHsc)
    else()
        target_compile_options(export_dense_matrix PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(test_dense_matrix tests/linalg/test_dense_matrix.cpp
        src/linalg/dense_matrix.cpp src/linalg/vector.cpp)
    target_include_directories(test_dense_matrix PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(test_dense_matrix PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    if(MSVC)
        target_compile_options(test_dense_matrix PRIVATE /W4 /utf-8 /EHsc)
    else()
        target_compile_options(test_dense_matrix PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    foreach(matrix_case IN ITEMS construction initial_value access const_access
            bounds_mutable bounds_const empty_shapes copy_assignment size_overflow
            transpose_rectangular transpose_empty matvec_rectangular matvec_special
            matvec_empty matvec_mismatch matmul_rectangular matmul_identity_zero
            matmul_self matmul_empty matmul_mismatch matmul_result_overflow)
        add_test(NAME dense_matrix_${matrix_case} COMMAND test_dense_matrix ${matrix_case})
        set_tests_properties(dense_matrix_${matrix_case} PROPERTIES LABELS "linalg;dense_matrix" TIMEOUT 10)
    endforeach()

    add_executable(export_vector tests/linalg/export_vector.cpp src/linalg/vector.cpp)
    target_include_directories(export_vector PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(export_vector PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    if(MSVC)
        target_compile_options(export_vector PRIVATE /W4 /utf-8 /EHsc)
    else()
        target_compile_options(export_vector PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(test_vector tests/linalg/test_vector.cpp src/linalg/vector.cpp "src/linalg/dense_matrix.cpp")
    target_include_directories(test_vector PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(test_vector PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    if(MSVC)
        target_compile_options(test_vector PRIVATE /W4 /utf-8 /EHsc)
    else()
        target_compile_options(test_vector PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    foreach(vector_case IN ITEMS construction access copy assignment arithmetic scaling
            dot norm2 norm_large norm_small norm_mixed_scales norm_nan norm_infinity
            negation axpy empty_operations mismatched_sizes bounds_mutable bounds_const)
        add_test(NAME vector_${vector_case} COMMAND test_vector ${vector_case})
        set_tests_properties(vector_${vector_case} PROPERTIES LABELS "linalg;vector" TIMEOUT 10)
    endforeach()
    add_test(NAME minicae_startup COMMAND minicae_demo)
    set_tests_properties(minicae_startup PROPERTIES
        PASS_REGULAR_EXPRESSION "MiniCAE-Solvers [(]${MINICAE_PLATFORM} ${MINICAE_CONFIGURATION}[)]: C[+][+]17 startup OK"
    )
endif()
