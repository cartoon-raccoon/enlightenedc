cmake_minimum_required(VERSION 3.20)
project(enlightenedc)

# 1. Setting various requirements and variables

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# 2. Finding our dependencies

find_package(BISON 3.8 REQUIRED)
find_package(FLEX 2.6 REQUIRED)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# 3. Define Flex/Bison targets

set(BISON_OUTPUT_DIR "${CMAKE_BINARY_DIR}/gen")
file(MAKE_DIRECTORY ${BISON_OUTPUT_DIR})

bison_target(
    parser
    "${SRC_DIR}/frontend/parser.y"
    "${BISON_OUTPUT_DIR}/parser.cpp"
    DEFINES_FILE "${BISON_OUTPUT_DIR}/parser.hpp"
)

flex_target(
    scanner
    "${SRC_DIR}/frontend/lexer.l"
    "${BISON_OUTPUT_DIR}/lexer.cpp"
)

add_flex_bison_dependency(scanner parser)

# 4. Collect source files
file(GLOB_RECURSE AST_SRC "${SRC_DIR}/ast/*.cpp")
file(GLOB_RECURSE PARSER_SRC "${SRC_DIR}/parser/*.cpp")
file(GLOB_RECURSE COMPILER_SRC "${SRC_DIR}/*.cpp")

# 5. Create our target executable
add_executable(ecc 
    ${PARSER_SRC}
    ${COMPILER_SRC} 
    ${BISON_parser_OUTPUTS} 
    ${FLEX_scanner_OUTPUTS} 
    "${SRC_DIR}/ecc.cpp"
)

# 6. Setup include paths
target_include_directories(ecc PRIVATE ${SRC_DIR})

# 7. Link LLVM libraries
llvm_map_components_to_libnames(llvm_libs
    core
    executionengine
    orcjit
    native
    support
)
target_link_libraries(ecc PRIVATE ${llvm_libs})