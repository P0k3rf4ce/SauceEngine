# Animation Tests CMakeLists.txt

# Create test executable for DerivFunc tests
add_executable(DerivFuncTests DerivFuncTests.cpp)

# Include directories
target_include_directories(DerivFuncTests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(DerivFuncTests PRIVATE
    animationLib
    GTest::gtest
    GTest::gtest_main
)

# Add test to CTest
add_test(NAME DerivFuncTests COMMAND DerivFuncTests)