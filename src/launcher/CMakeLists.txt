file(GLOB LAUNCHER_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB LAUNCHER_HDR_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/launcher/*.hpp")

add_library(launcherLib SHARED ${LAUNCHER_SRC_FILES} ${LAUNCHER_HDR_FILES})

set_target_properties(launcherLib PROPERTIES
  AUTOMOC ON
)

target_include_directories(launcherLib PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(launcherLib PRIVATE glfw)

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(launcherLib PRIVATE Boost::program_options)

find_package(Qt6Widgets CONFIG REQUIRED)
target_link_libraries(launcherLib PRIVATE Qt6::Widgets)

# Suppress duplicate library warnings from Qt6 static plugins on macOS
if(APPLE)
    target_link_options(launcherLib PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

# Deploy Qt runtime dependencies (DLLs and plugins)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt dependencies..."
        COMMAND Qt6::windeployqt --no-compiler-runtime --no-translations "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running windeployqt to deploy Qt dependencies"
    )
endif()

