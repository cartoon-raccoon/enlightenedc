include(GoogleTest)

function(ec_unit_test TARG SRCFILE)
    cmake_parse_arguments(ARG "RC" "" "" ${ARGN})

    add_executable(${TARG} "${CMAKE_CURRENT_SOURCE_DIR}/${SRCFILE}")

    set(LINK_LIBS GTest::gtest_main libecc)
    if(ARG_RC)
        list(APPEND LINK_LIBS rapidcheck)
    endif()

    target_link_libraries(${TARG} ${LINK_LIBS})
    set_target_properties(${TARG} PROPERTIES LABELS unit)
    gtest_discover_tests(${TARG} PROPERTIES LABELS unit)
endfunction()

ec_unit_test("typesystemtest" "typesystem_tests.cpp")
ec_unit_test("symboltabletest" "symboltable_tests.cpp")
ec_unit_test("valuetest" "value_tests.cpp" RC)
ec_unit_test("primitivetest" "primitive_tests.cpp" RC)
ec_unit_test("inheritancetest" "inheritance_tests.cpp")

