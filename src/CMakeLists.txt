include_directories(vulkan)

set(my-gui_SRC
  main.cc
  logger.cc
  window_mgr.cc
  draw_list.cc
  vulkan_render_device.cc
  vulkan_ctx.cc
  vulkan_canvas.cc
  # vulkan/window.cpp
  )

find_package(Threads REQUIRED)
find_package(rxcpp CONFIG)
find_package(SDL2 REQUIRED)
# find_package(glfw3 REQUIRED)
find_package(Boost REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(my-gui ${my-gui_SRC})
target_link_libraries(my-gui
  Threads::Threads
  ${CMAKE_DL_LIBS}
  # glfw
  glm
  rxcpp
  SDL2::SDL2
  Boost::headers
  ${FFMPEG_LIBRARIES}
  Vulkan::Vulkan
  )

file(GLOB shaders
  vulkan/shaders/shader.vert
  vulkan/shaders/shader.frag
  vulkan/shaders/canvas_shader.vert
  vulkan/shaders/canvas_shader.frag)

set(GLSL_VALIDATOR "/bin/glslangValidator")

foreach(glsl ${shaders})
  get_filename_component(file_name ${glsl} NAME)
  set(spirv ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${file_name}.spv)
  add_custom_command(
    OUTPUT ${spirv}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/
    COMMAND ${GLSL_VALIDATOR} -V ${glsl} -o ${spirv}
    DEPENDS ${glsl})
  list(APPEND spirv_binary_files ${spirv})
endforeach(glsl)

add_custom_target(
  shaders
  DEPENDS ${spirv_binary_files})

add_dependencies(my-gui shaders)
