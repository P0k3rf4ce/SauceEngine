file(GLOB LAUNCHER_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB LAUNCHER_HDR_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/launcher/*.hpp")

add_library(launcherLib STATIC ${LAUNCHER_SRC_FILES} ${LAUNCHER_HDR_FILES})

set_target_properties(launcherLib PROPERTIES
  AUTOMOC ON
)

target_include_directories(launcherLib PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)

find_package(glad CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(Eigen3 CONFIG REQUIRED NO_MODULE)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(launcherLib PRIVATE Boost::program_options)

find_package(Qt6Widgets CONFIG REQUIRED)
target_link_libraries(launcherLib PRIVATE Qt6::Widgets)

target_link_libraries(launcherLib PRIVATE sharedLib)
