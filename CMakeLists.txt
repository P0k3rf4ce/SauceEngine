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

function(configure_macos_app_icon target icon_path bundle_name bundle_identifier)
  if(NOT APPLE OR NOT EXISTS "${icon_path}")
    return()
  endif()

  get_filename_component(icon_name "${icon_path}" NAME)
  set_source_files_properties("${icon_path}" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )

  target_sources(${target} PRIVATE "${icon_path}")
  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "${bundle_name}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${bundle_identifier}"
    MACOSX_BUNDLE_ICON_FILE "${icon_name}"
  )
endfunction()

function(configure_windows_app_icon target icon_path)
  if(NOT WIN32 OR NOT EXISTS "${icon_path}")
    return()
  endif()

  set(resource_file "${CMAKE_CURRENT_BINARY_DIR}/${target}_icon.rc")
  set(APP_ICON_PATH "${icon_path}")
  configure_file("${PROJECT_SOURCE_DIR}/cmake/AppIcon.rc.in" "${resource_file}" @ONLY)
  target_sources(${target} PRIVATE "${resource_file}")
endfunction()

function(configure_macos_cli_symlink target cli_path)
  if(NOT APPLE)
    return()
  endif()

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f "${cli_path}"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:${target}>" "${cli_path}"
    VERBATIM
  )
endfunction()

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/app/*.cpp"
)

file(GLOB_RECURSE PHYSICS_SOURCES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/physics/*.cpp"
)

file(GLOB_RECURSE MODELING_SOURCES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/app/modeling/*.cpp"
)

file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/editor/*.cpp"
)

find_package(Vulkan REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

set(SAUCE_WARNINGS -Wall -Wpedantic -Wextra -Wreorder-init-list -g -O2)

add_subdirectory(${PROJECT_SOURCE_DIR}/shaders)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)

set(EXEC_NAME SauceEngine)
add_executable(${EXEC_NAME} src/main.cpp)

target_sources(${EXEC_NAME} PRIVATE
  ${APP_SOURCES}
  ${PHYSICS_SOURCES}
)

target_include_directories(${EXEC_NAME} PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(${EXEC_NAME}
  PUBLIC Vulkan::Vulkan Boost::program_options
  PRIVATE glfw imgui::imgui nlohmann_json::nlohmann_json launcherLib
)

if(NOT WIN32)
  target_compile_options(${EXEC_NAME} PRIVATE ${SAUCE_WARNINGS})
endif()

add_dependencies(${EXEC_NAME} shaders)

add_custom_command(TARGET ${EXEC_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${PROJECT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${EXEC_NAME}>/shaders
)

configure_macos_app_icon(${EXEC_NAME}
  "${PROJECT_SOURCE_DIR}/assets/sauceengine.icns"
  "SauceEngine"
  "com.sauceengine.app"
)
configure_windows_app_icon(${EXEC_NAME}
  "${PROJECT_SOURCE_DIR}/assets/sauceengine.ico"
)
configure_macos_cli_symlink(${EXEC_NAME}
  "${PROJECT_BINARY_DIR}/SauceEngine"
)

if(APPLE)
  target_link_options(${EXEC_NAME} PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

add_executable(modeltest src/modeltest.cpp)

target_sources(modeltest PRIVATE
  src/app/components/MeshRendererComponent.cpp
  ${MODELING_SOURCES}
)

target_include_directories(modeltest PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(modeltest PUBLIC Vulkan::Vulkan)

if(NOT WIN32)
  target_compile_options(modeltest PRIVATE ${SAUCE_WARNINGS})
endif()

add_executable(modelviewer src/modelviewer.cpp)

target_sources(modelviewer PRIVATE
  src/app/ModelViewerRenderer.cpp
  src/app/ImGuiRenderer.cpp
  src/app/components/MeshRendererComponent.cpp
  src/app/components/RigidBodyComponent.cpp
  ${MODELING_SOURCES}
)

target_include_directories(modelviewer PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(modelviewer
  PUBLIC Vulkan::Vulkan
  PRIVATE glfw imgui::imgui
)

if(NOT WIN32)
  target_compile_options(modelviewer PRIVATE ${SAUCE_WARNINGS})
endif()

add_dependencies(modelviewer shaders)

add_custom_command(TARGET modelviewer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${PROJECT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:modelviewer>/shaders
)

add_executable(jenga_scene_builder
  src/jenga_scene_builder.cpp
  src/app/Scene.cpp
  src/app/components/ClothComponent.cpp
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
  src/physics/Cloth.cpp
)

target_include_directories(jenga_scene_builder PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(jenga_scene_builder PUBLIC Vulkan::Vulkan)

if(NOT WIN32)
  target_compile_options(jenga_scene_builder PRIVATE ${SAUCE_WARNINGS})
endif()

add_custom_target(generate_jenga_scene
  COMMAND $<TARGET_FILE:jenga_scene_builder>
  DEPENDS jenga_scene_builder
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_executable(xpbd_cloth_harness
  src/xpbd_cloth_harness.cpp
  src/app/components/ClothComponent.cpp
  src/app/components/MeshRendererComponent.cpp
  src/app/components/TransformComponent.cpp
  src/app/modeling/Mesh.cpp
  src/app/modeling/Transform.cpp
  src/physics/Cloth.cpp
  src/physics/SphereCollider.cpp
  src/physics/XPBD.cpp
)

target_include_directories(xpbd_cloth_harness PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(xpbd_cloth_harness PUBLIC Vulkan::Vulkan)

if(NOT WIN32)
  target_compile_options(xpbd_cloth_harness PRIVATE ${SAUCE_WARNINGS})
endif()

add_test(NAME xpbd_cloth_harness COMMAND xpbd_cloth_harness)

add_executable(cloth_scene_smoke src/cloth_scene_smoke.cpp)

target_sources(cloth_scene_smoke PRIVATE ${APP_SOURCES} ${PHYSICS_SOURCES})

target_include_directories(cloth_scene_smoke PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(cloth_scene_smoke
    PUBLIC  Vulkan::Vulkan
    PRIVATE glfw imgui::imgui nlohmann_json::nlohmann_json
)

if(NOT WIN32)
  target_compile_options(cloth_scene_smoke PRIVATE ${SAUCE_WARNINGS})
endif()

add_test(NAME cloth_scene_smoke COMMAND cloth_scene_smoke)

add_executable(xpbd_rigid_harness
  src/xpbd_rigid_harness.cpp
  src/app/Scene.cpp
  src/app/PhysicsDemoSetup.cpp
  src/app/components/ClothComponent.cpp
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
  src/physics/Cloth.cpp
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
  target_compile_options(xpbd_rigid_harness PRIVATE ${SAUCE_WARNINGS})
endif()

add_test(NAME xpbd_stack_stability COMMAND xpbd_rigid_harness --test-stack-stability
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_test(NAME xpbd_piecewise_transfer COMMAND xpbd_rigid_harness --test-piecewise-transfer
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(sauceeditor ${EDITOR_SOURCES})

target_sources(sauceeditor PRIVATE
  ${APP_SOURCES}
  ${PHYSICS_SOURCES}
)

target_include_directories(sauceeditor PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(sauceeditor
  PUBLIC Vulkan::Vulkan
  PRIVATE glfw imgui::imgui nlohmann_json::nlohmann_json
)

if(NOT WIN32)
  target_compile_options(sauceeditor PRIVATE ${SAUCE_WARNINGS})
endif()

if(TARGET editorGridVert)
  add_dependencies(sauceeditor editorGridVert editorGridFrag editorUnlitVert editorUnlitFrag)
endif()

add_dependencies(sauceeditor shaders)

add_custom_command(TARGET sauceeditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${PROJECT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:sauceeditor>/shaders
)

configure_macos_app_icon(sauceeditor
  "${PROJECT_SOURCE_DIR}/assets/sauceeditor.icns"
  "SauceEditor"
  "com.sauceengine.editor"
)
configure_windows_app_icon(sauceeditor
  "${PROJECT_SOURCE_DIR}/assets/sauceeditor.ico"
)
configure_macos_cli_symlink(sauceeditor
  "${PROJECT_BINARY_DIR}/sauceeditor"
)
