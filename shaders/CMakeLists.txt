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

find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/bin")
if(!SLANGC_EXECUTABLE)
  message(FATAL_ERROR "slangc not found!\nIt should be part of Vulkan SDK; alternatively, install from: https://github.com/shader-slang/slang/releases")
endif()

add_slang_shader_target(shaderTarget SOURCES shader.slang OUTPUT shader.spv)
add_slang_shader_target(shaderLightsTarget SOURCES shader_lights.slang OUTPUT shader_lights.spv)
add_slang_shader_target(postProcessTarget SOURCES postprocess.slang OUTPUT postprocess.spv)
add_slang_shader_target(shaderPBRTarget SOURCES shader_pbr.slang OUTPUT shader_pbr.spv)
add_slang_shader_target(shadowMapTarget SOURCES shadow_map.slang OUTPUT shadow_map.spv)
add_slang_shader_target(shadowMapOmniTarget SOURCES shadow_map_omni.slang OUTPUT shadow_map_omni.spv)

add_slang_compute_target(iblBrdfLutTarget SOURCES ibl_brdf_lut.slang OUTPUT ibl_brdf_lut.spv)
add_slang_compute_target(iblIrradianceTarget SOURCES ibl_irradiance.slang OUTPUT ibl_irradiance.spv)
add_slang_compute_target(iblPrefilterTarget SOURCES ibl_prefilter.slang OUTPUT ibl_prefilter.spv)

# GLSL editor shaders compiled with glslc
# Prefer the imported target from find_package(Vulkan), fall back to PATH / VULKAN_SDK
if(TARGET Vulkan::glslc)
  set(GLSLC Vulkan::glslc)
else()
  find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/bin")
  if(NOT GLSLC)
    message(WARNING "glslc not found - editor shaders will not be compiled")
  endif()
endif()

function(add_glsl_shader_target TARGET SOURCE OUTPUT)
  add_custom_command(
    OUTPUT ${SHADERS_DIR}/${OUTPUT}
    COMMAND ${GLSLC} ${SHADERS_DIR}/${SOURCE} -o ${SHADERS_DIR}/${OUTPUT}
    DEPENDS ${SHADERS_DIR}/${SOURCE}
    COMMENT "Compiling GLSL shader ${SOURCE} -> ${OUTPUT}"
    VERBATIM
  )
  add_custom_target(${TARGET} DEPENDS ${SHADERS_DIR}/${OUTPUT})
endfunction()

if(GLSLC)
  add_glsl_shader_target(editorGridVert editor_grid.vert editor_grid.vert.spv)
  add_glsl_shader_target(editorGridFrag editor_grid.frag editor_grid.frag.spv)
  add_glsl_shader_target(editorUnlitVert editor_unlit.vert editor_unlit.vert.spv)
  add_glsl_shader_target(editorUnlitFrag editor_unlit.frag editor_unlit.frag.spv)
  add_glsl_shader_target(editorLitVert editor_lit.vert editor_lit.vert.spv)
  add_glsl_shader_target(editorLitFrag editor_lit.frag editor_lit.frag.spv)
endif()

add_slang_compute_target(iblEquirectToCubeTarget SOURCES ibl_equirect_to_cube.slang OUTPUT ibl_equirect_to_cube.spv)

add_custom_target(shaders DEPENDS
  shaderTarget
  shaderLightsTarget
  postProcessTarget
  shaderPBRTarget
  shadowMapTarget
  shadowMapOmniTarget
  iblBrdfLutTarget
  iblIrradianceTarget
  iblPrefilterTarget
  iblEquirectToCubeTarget
  $<$<BOOL:${GLSLC}>:editorGridVert>
  $<$<BOOL:${GLSLC}>:editorGridFrag>
  $<$<BOOL:${GLSLC}>:editorUnlitVert>
  $<$<BOOL:${GLSLC}>:editorUnlitFrag>
  $<$<BOOL:${GLSLC}>:editorLitVert>
  $<$<BOOL:${GLSLC}>:editorLitFrag>
)
