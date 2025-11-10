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

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::program_options)

find_package(Qt6Widgets CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

# Deploy Qt runtime dependencies (DLLs and plugins)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt dependencies..."
        COMMAND Qt6::windeployqt --no-compiler-runtime --no-translations "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running windeployqt to deploy Qt dependencies"
    )
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/src/animation)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/modeling)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/rendering)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/shared)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/utils)

target_link_libraries(${PROJECT_NAME} PUBLIC launcherLib utilsLib animationLib modelingLib renderingLib sharedLib)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(modelingLib PRIVATE assimp::assimp)

if (BUILD_TESTS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif()