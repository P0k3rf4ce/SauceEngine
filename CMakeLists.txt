# CMakeLists.txt (top of file, BEFORE project())
cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SauceEngine)

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

add_executable(SauceEngine src/main.cpp)

target_include_directories(SauceEngine PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

find_package(glad CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(Eigen3 CONFIG REQUIRED NO_MODULE)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/animation)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/modeling)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/rendering)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/shared)

target_link_libraries(${PROJECT_NAME} PRIVATE sharedLib animationLib modelingLib renderingLib)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(modelingLib PRIVATE assimp::assimp)

if (BUILD_TESTS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif()