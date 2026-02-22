set(SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR})

function (add_slang_shader_target TARGET)
  cmake_parse_arguments (arg_shader "" "OUTPUT" "SOURCES" ${ARGN})

  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  add_custom_command (
          OUTPUT ${arg_shader_OUTPUT}
          COMMAND slangc ${arg_shader_SOURCES} -verbose-paths -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${arg_shader_OUTPUT}
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${arg_shader_SOURCES}
          COMMENT "Compiling Slang Shader ${arg_shader_OUTPUT} with sources ${arg_shader_SOURCES}"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${arg_shader_OUTPUT})
endfunction()

function (add_slang_compute_target TARGET)
  cmake_parse_arguments (arg_shader "" "OUTPUT" "SOURCES" ${ARGN})

  add_custom_command (
          OUTPUT ${arg_shader_OUTPUT}
          COMMAND slangc ${arg_shader_SOURCES} -verbose-paths -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry computeMain -o ${arg_shader_OUTPUT}
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${arg_shader_SOURCES}
          COMMENT "Compiling Slang Compute Shader ${arg_shader_OUTPUT} with sources ${arg_shader_SOURCES}"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${arg_shader_OUTPUT})
endfunction()

find_program(SLANGC_EXECUTABLE slangc)
if(!SLANGC_EXECUTABLE)
  message(FATAL_ERROR "slangc not found!\nIt should be part of Vulkan SDK; alternatively, install from: https://github.com/shader-slang/slang/releases")
endif()

add_slang_shader_target(shaderTarget SOURCES shader.slang OUTPUT shader.spv)
add_slang_shader_target(shaderLightsTarget SOURCES shader_lights.slang OUTPUT shader_lights.spv)
add_slang_shader_target(postProcessTarget SOURCES postprocess.slang OUTPUT postprocess.spv)
add_slang_shader_target(shaderPBRTarget SOURCES shader_pbr.slang OUTPUT shader_pbr.spv)

add_slang_compute_target(iblBrdfLutTarget SOURCES ibl_brdf_lut.slang OUTPUT ibl_brdf_lut.spv)
add_slang_compute_target(iblIrradianceTarget SOURCES ibl_irradiance.slang OUTPUT ibl_irradiance.spv)
add_slang_compute_target(iblPrefilterTarget SOURCES ibl_prefilter.slang OUTPUT ibl_prefilter.spv)

add_custom_target(shaders DEPENDS
  shaderTarget
  shaderLightsTarget
  postProcessTarget
  shaderPBRTarget
  iblBrdfLutTarget
  iblIrradianceTarget
  iblPrefilterTarget
)

