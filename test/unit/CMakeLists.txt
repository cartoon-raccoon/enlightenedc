add_executable(
    typesystemtest 
    "${CMAKE_CURRENT_SOURCE_DIR}/typesystem_tests.cpp"
)
target_link_libraries(
    typesystemtest GTest::gtest_main libecc
)
set_target_properties(typesystemtest PROPERTIES LABELS unit)

include(GoogleTest)
gtest_discover_tests(
    typesystemtest
    PROPERTIES LABELS unit
)