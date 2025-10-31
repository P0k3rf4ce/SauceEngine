enable_testing()

find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB TEST_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/**/*_test.cpp")

add_executable(unit_tests ${TEST_FILES})

target_link_libraries(unit_tests PRIVATE sharedLib animationLib modelingLib renderingLib)

target_include_directories(unit_tests
  PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(unit_tests PRIVATE GTest::gtest GTest::gtest_main)

find_package(glad CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(Eigen3 CONFIG REQUIRED NO_MODULE)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)

if (APPLE)
    gtest_discover_tests(unit_tests PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage/%p-%m.profraw")
else()
    gtest_discover_tests(unit_tests)
endif()
