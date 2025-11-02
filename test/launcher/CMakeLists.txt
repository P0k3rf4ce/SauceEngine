enable_testing()

find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB TEST_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp")

add_executable(launcher_unit_tests ${TEST_FILES})

target_link_libraries(launcher_unit_tests PRIVATE launcherLib)

target_include_directories(launcher_unit_tests
  PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(launcher_unit_tests PRIVATE GTest::gtest GTest::gtest_main)

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::program_options)

gtest_discover_tests(launcher_unit_tests)