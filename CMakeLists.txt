cmake_minimum_required(VERSION 3.30)

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

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${EXEC_NAME} PRIVATE nlohmann_json::nlohmann_json)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(${EXEC_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})

target_include_directories(SauceEngine PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

file(GLOB_RECURSE UI_COMPONENT_SOURCES
  "${PROJECT_SOURCE_DIR}/src/app/ui/*.cpp" 
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
  src/app/modeling/GLTFExporter.cpp
  src/app/components/TransformComponent.cpp
  src/app/components/RigidBodyComponent.cpp
  src/app/components/MeshRendererComponent.cpp
  src/app/components/LightComponent.cpp
  src/app/components/PointLightComponent.cpp
  src/app/components/SpotLightComponent.cpp
  src/app/components/DirectionalLightComponent.cpp
  src/app/Scene.cpp
  src/app/ImGuiRenderer.cpp
  src/app/SauceEngineApp.cpp
  src/app/IBLGenerator.cpp
  src/app/ui/ImGuiComponentManager.cpp
  ${UI_COMPONENT_SOURCES}
)

add_subdirectory(${PROJECT_SOURCE_DIR}/shaders)
# add_dependencies(${EXEC_NAME} shaders)  # Requires slangc; pre-compiled .spv files used instead

add_custom_command(TARGET ${EXEC_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${EXEC_NAME}>/shaders
)

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
    src/app/components/RigidBodyComponent.cpp
)

target_link_libraries(modelviewer PUBLIC Vulkan::Vulkan)
target_link_libraries(modelviewer PRIVATE glfw)
target_link_libraries(modelviewer PRIVATE imgui::imgui)

add_dependencies(modelviewer shaders)

add_custom_command(TARGET modelviewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:modelviewer>/shaders
)

# SauceEditor executable
if (NOT WIN32)
add_executable(sauceeditor src/editor/main.cpp)

target_include_directories(sauceeditor PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

if (!WIN32)
    target_compile_options(sauceeditor PUBLIC -Wall -Wpedantic -Wextra -Wreorder-init-list -g -O2)
endif()

file(GLOB_RECURSE EDITOR_SOURCES
    "${PROJECT_SOURCE_DIR}/src/editor/*.cpp"
)

target_sources(sauceeditor PRIVATE
    ${EDITOR_SOURCES}
    src/app/modeling/Transform.cpp
    src/app/modeling/Texture.cpp
    src/app/modeling/TextureCache.cpp
    src/app/modeling/Material.cpp
    src/app/modeling/Mesh.cpp
    src/app/modeling/ModelNode.cpp
    src/app/modeling/Model.cpp
    src/app/modeling/GLTFLoader.cpp
    src/app/modeling/GLTFExporter.cpp
    src/app/components/TransformComponent.cpp
    src/app/components/RigidBodyComponent.cpp
    src/app/components/MeshRendererComponent.cpp
    src/app/components/LightComponent.cpp
    src/app/components/PointLightComponent.cpp
    src/app/components/SpotLightComponent.cpp
    src/app/components/DirectionalLightComponent.cpp
    src/app/Scene.cpp
    src/app/ImGuiRenderer.cpp
    src/app/Log.cpp
    src/app/Settings.cpp
)

target_link_libraries(sauceeditor PUBLIC Vulkan::Vulkan)
target_link_libraries(sauceeditor PRIVATE glfw)
target_link_libraries(sauceeditor PRIVATE imgui::imgui)
target_link_libraries(sauceeditor PRIVATE nlohmann_json::nlohmann_json)

# Editor only needs GLSL shaders, not the Slang ones
if(TARGET editorGridVert)
  add_dependencies(sauceeditor editorGridVert editorGridFrag editorUnlitVert editorUnlitFrag)
endif()
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)

target_link_libraries(${EXEC_NAME} PRIVATE launcherLib)

if(APPLE)
    target_link_options(${EXEC_NAME} PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

