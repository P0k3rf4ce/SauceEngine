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

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${EXEC_NAME} PRIVATE imgui::imgui)

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
  src/app/ImGuiRenderer.cpp
  src/app/ui/ImGuiComponentManager.cpp
)

set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)

function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  add_custom_command (
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  add_custom_command (
          OUTPUT  ${SHADERS_DIR}/slang.spv
          COMMAND slangc ${SHADER_SOURCES} -verbose-paths -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

find_program(SLANGC_EXECUTABLE slangc)
if(SLANGC_EXECUTABLE)
  message(STATUS "Found slangc: ${SLANGC_EXECUTABLE}")
  add_slang_shader_target(shadersTarget SOURCES ${SHADERS_DIR}/shader.slang)
  add_dependencies(${EXEC_NAME} shadersTarget)

  # Compile 3D shader for model viewer
  set(ENTRY_POINTS -entry vertMain -entry fragMain)
  add_custom_command(
    OUTPUT ${SHADERS_DIR}/slang_3d.spv
    COMMAND slangc ${SHADERS_DIR}/shader_3d.slang -verbose-paths -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang_3d.spv
    WORKING_DIRECTORY ${SHADERS_DIR}
    DEPENDS ${SHADERS_DIR}/shader_3d.slang
    COMMENT "Compiling 3D Slang Shader"
    VERBATIM
  )
  add_custom_target(shaders3DTarget DEPENDS ${SHADERS_DIR}/slang_3d.spv)
else()
  message(WARNING "slangc not found - skipping shader compilation. Install from: https://github.com/shader-slang/slang/releases")
  # Create empty shader target so build doesn't fail
  add_custom_target(shadersTarget)
  add_custom_target(shaders3DTarget)
endif()

# Model test executable
add_executable(modeltest src/modeltest.cpp)
target_include_directories(modeltest PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(modeltest PRIVATE ${TINYGLTF_INCLUDE_DIRS})

if (!WIN32)
  target_compile_options(modeltest PUBLIC -Wall -Wpedantic -Wextra -Wreorder-init-list -g -O2)
endif()

target_sources(modeltest PRIVATE
  src/app/modeling/Transform.cpp
  src/app/modeling/Texture.cpp
  src/app/modeling/TextureCache.cpp
  src/app/modeling/Material.cpp
  src/app/modeling/Mesh.cpp
  src/app/modeling/ModelNode.cpp
  src/app/modeling/Model.cpp
  src/app/modeling/GLTFLoader.cpp
)

# We need Vulkan headers (but won't initialize Vulkan)
target_link_libraries(modeltest PUBLIC Vulkan::Vulkan)

# ModelViewer executable
add_executable(modelviewer src/modelviewer.cpp)

target_include_directories(modelviewer PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

if (!WIN32)
    target_compile_options(modelviewer PUBLIC -Wall -Wpedantic -Wextra -Wreorder-init-list -g -O2)
endif()

target_sources(modelviewer PRIVATE
    src/app/ModelViewerRenderer.cpp
    src/app/modeling/Transform.cpp
    src/app/modeling/Texture.cpp
    src/app/modeling/TextureCache.cpp
    src/app/modeling/Material.cpp
    src/app/modeling/Mesh.cpp
    src/app/modeling/ModelNode.cpp
    src/app/modeling/Model.cpp
    src/app/modeling/GLTFLoader.cpp
    src/app/ImGuiRenderer.cpp
)

target_link_libraries(modelviewer PUBLIC Vulkan::Vulkan)
target_link_libraries(modelviewer PRIVATE glfw)
target_link_libraries(modelviewer PRIVATE imgui::imgui)

add_dependencies(modelviewer shaders3DTarget)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)

target_link_libraries(${EXEC_NAME} PRIVATE launcherLib)

if(APPLE)
    target_link_options(${EXEC_NAME} PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

