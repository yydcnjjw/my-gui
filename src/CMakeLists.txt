set(COMPILE_FLAGS "-Wall -Wextra -ggdb3 -Fsanitize=Address")

set(CMAKE_C_FLAGS ${COMPILE_FLAGS})
set(CMAKE_CXX_FLAGS ${COMPILE_FLAGS})

message("${msdfgen_inc}")
message("${msdfgen_lib}")
include_directories(
  vulkan
  ${msdfgen_inc})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(my-gui_SRC
  main.cc
  application.cc
  logger.cc
  window_mgr.cc
  render_server.cc
  font_mgr.cc
  resource_mgr.cc
  draw_list.cc
  async_task.cc
  event_bus.cc
  # vulkan_render_device.cc
  vulkan_ctx.cc
  vulkan_canvas.cc
  storage/xp3_archive.cc
  # vulkan/window.cpp
  )

find_package(Threads REQUIRED)
find_package(rxcpp CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
# find_package(glfw3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread program_options iostreams locale)
find_package(FFMPEG REQUIRED)
find_package(Freetype REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG)
find_package(ZLIB REQUIRED)

set(DEPS
  Threads::Threads
  ${CMAKE_DL_LIBS}
  # glfw
  glm::glm
  rxcpp
  SDL2::SDL2
  # Boost::headers
  Boost::thread
  Boost::program_options
  Boost::iostreams
  Boost::locale
  ${FFMPEG_LIBRARIES}
  Freetype::Freetype
  ${msdfgen_lib}
  Vulkan::Vulkan
  # ZLIB::ZLIB
  )

add_executable(my-gui ${my-gui_SRC})
target_link_libraries(my-gui
  ${DEPS}
  )
add_library(my-gui_lib SHARED ${my-gui_SRC})
target_link_libraries(my-gui_lib
  ${DEPS}
  )

set(PROJECT_ASSETS_PATH ${PROJECT_SOURCE_DIR}/assets)
set(PROJECT_SHADER_PATH ${PROJECT_ASSETS_PATH}/shaders)
file(GLOB shaders
  ${PROJECT_SHADER_PATH}/shader.vert
  ${PROJECT_SHADER_PATH}/shader.frag
  ${PROJECT_SHADER_PATH}/canvas_shader.vert
  ${PROJECT_SHADER_PATH}/canvas_shader.frag)

set(GLSL_VALIDATOR "/bin/glslangValidator")

foreach(glsl ${shaders})
  get_filename_component(file_name ${glsl} NAME)
  set(spirv ${PROJECT_SHADER_PATH}/${file_name}.spv)
  add_custom_command(
    OUTPUT ${spirv}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SHADER_PATH}
    COMMAND ${GLSL_VALIDATOR} -V ${glsl} -o ${spirv}
    DEPENDS ${glsl})
  list(APPEND spirv_binary_files ${spirv})
endforeach(glsl)

add_custom_target(
  shaders
  DEPENDS ${spirv_binary_files})

add_dependencies(my-gui shaders)

add_custom_target(assets)
add_custom_command(
  TARGET assets
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_ASSETS_PATH} ${DEPLOY_DIR}
  )

add_dependencies(my-gui assets)
