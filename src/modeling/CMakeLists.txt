file(GLOB MODELING_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(modelingLib STATIC ${MODELING_SRC_FILES})

target_include_directories(modelingLib PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

find_package(glad CONFIG REQUIRED)
target_link_libraries(modelingLib PUBLIC glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(modelingLib PUBLIC glfw)

find_package(Eigen3 CONFIG REQUIRED NO_MODULE)
target_link_libraries(modelingLib PUBLIC Eigen3::Eigen)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(modelingLib PUBLIC assimp::assimp)

target_link_libraries(modelingLib PUBLIC utilsLib)