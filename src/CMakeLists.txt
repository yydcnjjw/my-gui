include_directories(vulkan)

set(my-gui_SRC
  main.cc
  logger.cc
  # vulkan_render_device.cc
  vulkan_ctx.cc
  window_mgr.cc
  # vulkan/my_vulkan.cpp
  # vulkan/vulkan_application.cpp
  # vulkan/vulkan_ext.cpp
  # vulkan/vulkan_init.cpp
  # vulkan/vulkan_instance.cpp
  # vulkan/window.cpp
  )

# find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(rxcpp CONFIG)
find_package(Threads REQUIRED)

add_executable(my-gui ${my-gui_SRC})
target_link_libraries(my-gui
  glfw
  glm
  Vulkan::Vulkan
  rxcpp
  Threads::Threads
  ${CMAKE_DL_LIBS}
  SDL2::SDL2
  )

file(GLOB shaders
  vulkan/shaders/shader.vert
  vulkan/shaders/shader.frag)

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
