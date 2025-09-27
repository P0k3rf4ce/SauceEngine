file(GLOB SHARED_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(utilsLib SHARED ${SHARED_SRC_FILES})

target_include_directories(utilsLib PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

find_package(glad CONFIG REQUIRED)
target_link_libraries(utilsLib PUBLIC glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(utilsLib PUBLIC glfw)

find_package(Eigen3 CONFIG REQUIRED NO_MODULE)
target_link_libraries(utilsLib PUBLIC Eigen3::Eigen)