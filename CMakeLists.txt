cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SauceEngine)

# Add coverage flags for Apple builds
# if(APPLE)
#     add_compile_options(-g -fprofile-instr-generate -fcoverage-mapping)
#     add_link_options(-fprofile-instr-generate -g)
# endif()

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

set(EXEC_NAME SauceEngine)
add_executable(SauceEngine src/main.cpp)

find_package(Vulkan REQUIRED)
target_link_libraries(${EXEC_NAME} PUBLIC Vulkan::Vulkan)

# find_package(glfw3 CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::program_options)

find_package(Qt6Widgets CONFIG REQUIRED)

# target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Widgets)
# 
# # Deploy Qt runtime dependencies (DLLs and plugins)
# if(WIN32)
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt dependencies..."
#         COMMAND Qt6::windeployqt --no-compiler-runtime --no-translations "$<TARGET_FILE:${PROJECT_NAME}>"
#         COMMENT "Running windeployqt to deploy Qt dependencies"
#     )
# endif()

target_include_directories(SauceEngine PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  $<TARGET_PROPERTY:Qt6::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
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

add_slang_shader_target(shadersTarget SOURCES ${SHADERS_DIR}/shader.slang)
add_dependencies(${EXEC_NAME} shadersTarget)

# add_subdirectory(${PROJECT_SOURCE_DIR}/src/animation)
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/modeling)
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/rendering)
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/shared)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/utils)
# add_subdirectory(external/osqp)

target_link_libraries(${EXEC_NAME} PRIVATE launcherLib) 

# Suppress duplicate library warnings from Qt6 static plugins on macOS
if(APPLE)
    target_link_options(${EXEC_NAME} PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

if (BUILD_TESTS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif()
