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

find_package(Stb REQUIRED)
target_include_directories(${EXEC_NAME} PRIVATE ${Stb_INCLUDE_DIR})

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::program_options)

find_package(Qt6Widgets CONFIG REQUIRED)

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

add_subdirectory(${PROJECT_SOURCE_DIR}/src/launcher)

target_link_libraries(${EXEC_NAME} PRIVATE launcherLib) 

if(APPLE)
    target_link_options(${EXEC_NAME} PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

