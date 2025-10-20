file(GLOB ANIMATION_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(animationLib SHARED ${ANIMATION_SRC_FILES})

target_include_directories(animationLib PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

find_package(glad CONFIG REQUIRED)
target_link_libraries(animationLib PUBLIC glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(animationLib PUBLIC glfw)

find_package(Eigen3 CONFIG REQUIRED NO_MODULE)
target_link_libraries(animationLib PUBLIC Eigen3::Eigen)

target_link_libraries(animationLib PUBLIC osqp)