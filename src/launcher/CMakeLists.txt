file(GLOB LAUNCHER_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(launcherLib STATIC ${LAUNCHER_SRC_FILES})

target_include_directories(launcherLib PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(launcherLib PRIVATE Boost::program_options)