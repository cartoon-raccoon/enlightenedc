add_test(
    NAME integration
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/run_test.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(integration PROPERTIES
    LABELS integration
    ENVIRONMENT "ECC_BIN=$<TARGET_FILE:ecc>"
)
