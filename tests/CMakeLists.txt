# Test configuration for deduplikate

find_package(Qt6 REQUIRED COMPONENTS Test)

# Helper macro for adding Qt tests
macro(add_deduplikate_test testname)
    add_executable(${testname} cpp/${testname}.cpp)

    # Link against core library and Qt6::Test
    target_link_libraries(${testname} PRIVATE
        deduplikate_core
        Qt6::Test
    )

    # Add the test to CTest
    add_test(NAME ${testname} COMMAND ${testname})

    # Set test properties
    set_tests_properties(${testname} PROPERTIES
        TIMEOUT 300
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endmacro()

# Add tests (uncomment as they are created)
add_deduplikate_test(test_duplicatemodel)
# add_deduplikate_test(test_duplicatefinder)
# add_deduplikate_test(test_mainwindow)
# add_deduplikate_test(test_integration)
# add_deduplikate_test(test_file_operations)
# add_deduplikate_test(test_settings_persistence)
# add_deduplikate_test(test_performance)
# add_deduplikate_test(test_edge_cases)
# add_deduplikate_test(test_error_handling)
