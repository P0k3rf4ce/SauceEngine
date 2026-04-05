file(GLOB LAUNCHER_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB LAUNCHER_HDR_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/launcher/*.hpp")

add_library(launcherLib STATIC ${LAUNCHER_SRC_FILES} ${LAUNCHER_HDR_FILES})

target_include_directories(launcherLib PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(launcherLib PRIVATE glfw)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(launcherLib PRIVATE imgui::imgui)

find_package(CURL REQUIRED)
target_link_libraries(launcherLib PRIVATE CURL::libcurl)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(launcherLib PRIVATE nlohmann_json::nlohmann_json)

target_link_libraries(launcherLib
  PUBLIC
    Vulkan::Vulkan
)

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(launcherLib PRIVATE Boost::program_options)
