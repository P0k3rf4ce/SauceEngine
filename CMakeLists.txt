cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(SauceEngine)

enable_testing()

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()




# ── Common sources (shared across multiple targets) ──────────────────

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/app/*.cpp"
)

file(GLOB_RECURSE PHYSICS_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/physics/*.cpp"
)

# Modeling-only subset (for lightweight targets like modeltest)
file(GLOB_RECURSE MODELING_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/app/modeling/*.cpp"
)

# ── Dependencies ─────────────────────────────────────────────────────

find_package(Vulkan REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

# ── Compiler warnings (non-Windows) ─────────────────────────────────

set(SAUCE_WARNINGS -Wall -Wpedantic -Wextra -Wreorder-init-list -g -O2)

# ── SauceEngine (main executable) ────────────────────────────────────

set(EXEC_NAME SauceEngine)
add_executable(${EXEC_NAME} src/main.cpp)

target_sources(${EXEC_NAME} PRIVATE ${APP_SOURCES} ${PHYSICS_SOURCES})

target_include_directories(${EXEC_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(${EXEC_NAME}
    PUBLIC  Vulkan::Vulkan Boost::program_options
    PRIVATE glfw imgui::imgui nlohmann_json::nlohmann_json launcherLib
)

if(NOT WIN32)
    target_compile_options(${EXEC_NAME} PUBLIC ${SAUCE_WARNINGS})
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/shaders)
add_dependencies(${EXEC_NAME} shaders)

add_custom_command(TARGET ${EXEC_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${EXEC_NAME}>/shaders
)

if(APPLE)
    target_link_options(${EXEC_NAME} PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

# ── jenga_scene_builder ──────────────────────────────────────────────

add_executable(jenga_scene_builder
    src/jenga_scene_builder.cpp
    src/app/Scene.cpp
    src/app/components/TransformComponent.cpp
    src/app/components/MeshRendererComponent.cpp
    src/app/components/PointLightComponent.cpp
    src/app/components/DirectionalLightComponent.cpp
    src/app/components/SpotLightComponent.cpp
    src/app/modeling/GLTFLoader.cpp
    src/app/modeling/GLTFExporter.cpp
    src/app/modeling/Model.cpp
    src/app/modeling/ModelNode.cpp
    src/app/modeling/Mesh.cpp
    src/app/modeling/Material.cpp
    src/app/modeling/Texture.cpp
    src/app/modeling/TextureCache.cpp
    src/app/modeling/Transform.cpp
)

target_include_directories(jenga_scene_builder PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(jenga_scene_builder
    PUBLIC Vulkan::Vulkan
)

if(NOT WIN32)
    target_compile_options(jenga_scene_builder PUBLIC ${SAUCE_WARNINGS})
endif()

add_custom_target(generate_jenga_scene
    COMMAND $<TARGET_FILE:jenga_scene_builder>
    DEPENDS jenga_scene_builder
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# ── modeltest ────────────────────────────────────────────────────────

# add_executable(modeltest src/modeltest.cpp)

# target_sources(modeltest PRIVATE
#     src/app/components/MeshRendererComponent.cpp
#     ${MODELING_SOURCES}
# )

# target_include_directories(modeltest PRIVATE
#     ${PROJECT_SOURCE_DIR}/include
#     ${TINYGLTF_INCLUDE_DIRS}
# )

# target_link_libraries(modeltest PUBLIC Vulkan::Vulkan)

# if(NOT WIN32)
#     target_compile_options(modeltest PUBLIC ${SAUCE_WARNINGS})
# endif()

# ── modelviewer ──────────────────────────────────────────────────────

# add_executable(modelviewer src/modelviewer.cpp)

# target_sources(modelviewer PRIVATE
#     src/app/ModelViewerRenderer.cpp
#     src/app/ImGuiRenderer.cpp
#     src/app/components/RigidBodyComponent.cpp
#     ${MODELING_SOURCES}
# )
# target_sources(modelviewer PRIVATE
#     src/app/ModelViewerRenderer.cpp
#     src/app/ImGuiRenderer.cpp
#     src/app/components/MeshRendererComponent.cpp
#     src/app/components/RigidBodyComponent.cpp
#     ${MODELING_SOURCES}
# )

# target_include_directories(modelviewer PRIVATE
#     ${PROJECT_SOURCE_DIR}/include
#     ${TINYGLTF_INCLUDE_DIRS}
# )

# target_link_libraries(modelviewer
#     PUBLIC  Vulkan::Vulkan
#     PRIVATE glfw imgui::imgui
# )

# if(NOT WIN32)
#     target_compile_options(modelviewer PUBLIC ${SAUCE_WARNINGS})
# endif()

# add_dependencies(modelviewer shaders)

# add_custom_command(TARGET modelviewer POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${PROJECT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:modelviewer>/shaders
# )

# ── xpbd_cloth_harness ───────────────────────────────────────────────

add_executable(xpbd_cloth_harness
    src/xpbd_cloth_harness.cpp
    src/app/components/MeshRendererComponent.cpp
    src/physics/Cloth.cpp
    src/physics/SphereBVH.cpp
    src/physics/SphereCollider.cpp
    src/physics/XPBD.cpp
    src/physics/XPBDContacts.cpp
    src/physics/XPBDRigidSolve.cpp
)

target_include_directories(xpbd_cloth_harness PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(xpbd_cloth_harness PUBLIC Vulkan::Vulkan)

if(NOT WIN32)
    target_compile_options(xpbd_cloth_harness PUBLIC ${SAUCE_WARNINGS})
endif()

add_test(NAME xpbd_cloth_harness COMMAND xpbd_cloth_harness)

# ── xpbd_rigid_harness ───────────────────────────────────────────────

add_executable(xpbd_rigid_harness
    src/xpbd_rigid_harness.cpp
    src/app/Scene.cpp
    src/app/PhysicsDemoSetup.cpp
    src/app/components/TransformComponent.cpp
    src/app/components/MeshRendererComponent.cpp
    src/app/components/RigidBodyComponent.cpp
    src/app/components/PointLightComponent.cpp
    src/app/components/DirectionalLightComponent.cpp
    src/app/components/SpotLightComponent.cpp
    src/app/modeling/GLTFLoader.cpp
    src/app/modeling/GLTFExporter.cpp
    src/app/modeling/Model.cpp
    src/app/modeling/ModelNode.cpp
    src/app/modeling/Mesh.cpp
    src/app/modeling/Material.cpp
    src/app/modeling/Texture.cpp
    src/app/modeling/TextureCache.cpp
    src/app/modeling/Transform.cpp
    src/physics/SphereBVH.cpp
    src/physics/SphereCollider.cpp
    src/physics/XPBD.cpp
    src/physics/XPBDContacts.cpp
    src/physics/XPBDRigidSolve.cpp
)

target_include_directories(xpbd_rigid_harness PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(xpbd_rigid_harness PUBLIC Vulkan::Vulkan)

if(NOT WIN32)
    target_compile_options(xpbd_rigid_harness PUBLIC ${SAUCE_WARNINGS})
endif()

# ── SauceEditor ──────────────────────────────────────────────────────

file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/editor/*.cpp"
)

add_executable(sauceeditor ${EDITOR_SOURCES})

target_sources(sauceeditor PRIVATE
    ${APP_SOURCES}
    ${PHYSICS_SOURCES}
    src/app/Log.cpp
    src/app/Settings.cpp
)

target_include_directories(sauceeditor PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(sauceeditor
    PUBLIC  Vulkan::Vulkan
    PRIVATE glfw imgui::imgui nlohmann_json::nlohmann_json
)

if(NOT WIN32)
    target_compile_options(sauceeditor PUBLIC ${SAUCE_WARNINGS})
endif()

if(TARGET editorGridVert)
    add_dependencies(sauceeditor editorGridVert editorGridFrag editorUnlitVert editorUnlitFrag)
endif()
add_dependencies(sauceeditor shaders)

# ── Launcher ─────────────────────────────────────────────────────────

add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)
