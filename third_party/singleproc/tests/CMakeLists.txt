add_executable(singleproc_tests test_guard.cpp)
add_executable(singleproc_doorbell_tests test_doorbell.cpp)
add_executable(singleproc_process_tests test_process.cpp)
add_dependencies(singleproc_process_tests singleproc_probe)
add_executable(singleproc_probe probe.cpp)

foreach(target singleproc_tests singleproc_doorbell_tests singleproc_process_tests singleproc_probe)
    target_link_libraries(${target} PRIVATE singleproc::singleproc)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

add_test(NAME singleproc_tests COMMAND singleproc_tests)
add_test(NAME singleproc_doorbell_tests COMMAND singleproc_doorbell_tests)
add_test(NAME singleproc_process_tests COMMAND singleproc_process_tests $<TARGET_FILE:singleproc_probe>)
add_test(NAME singleproc_install_consumer
    COMMAND ${CMAKE_COMMAND}
            -DSINGLEPROC_BINARY_DIR=${PROJECT_BINARY_DIR}
            -DSINGLEPROC_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            "-DSINGLEPROC_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DSINGLEPROC_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
            "-DSINGLEPROC_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/install_test.cmake)

set_tests_properties(singleproc_tests singleproc_doorbell_tests PROPERTIES TIMEOUT 30)
set_tests_properties(singleproc_process_tests PROPERTIES TIMEOUT 60)
set_tests_properties(singleproc_install_consumer PROPERTIES TIMEOUT 120)
