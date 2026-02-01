cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SauceEngine)

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

set(EXEC_NAME SauceEngine)
add_executable(${EXEC_NAME} src/main.cpp)

if (!WIN32) 
  target_compile_options(${EXEC_NAME} PUBLIC -Wall -Wpedantic -Wextra -Wreorder-init-list -g -O2)
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(${EXEC_NAME} PUBLIC Vulkan::Vulkan)

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(${EXEC_NAME} PUBLIC Boost::program_options)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${EXEC_NAME} PRIVATE glfw)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(${EXEC_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})

target_include_directories(SauceEngine PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

target_sources(${EXEC_NAME} PRIVATE
  src/app/modeling/Transform.cpp
  src/app/modeling/Texture.cpp
  src/app/modeling/TextureCache.cpp
  src/app/modeling/Material.cpp
  src/app/modeling/Mesh.cpp
  src/app/modeling/ModelNode.cpp
  src/app/modeling/Model.cpp
  src/app/modeling/GLTFLoader.cpp
  src/app/components/TransformComponent.cpp
  src/app/components/MeshRendererComponent.cpp
  src/app/Scene.cpp
)

add_subdirectory(${PROJECT_SOURCE_DIR}/shaders)
add_dependencies(${EXEC_NAME} shaders)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)

target_link_libraries(${EXEC_NAME} PRIVATE launcherLib)

if(APPLE)
    target_link_options(${EXEC_NAME} PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

