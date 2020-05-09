set(COMPILE_FLAGS "-Wall -Wextra")

set(CMAKE_C_FLAGS ${COMPILE_FLAGS})
set(CMAKE_CXX_FLAGS ${COMPILE_FLAGS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(my-gui_inc
  # build
  ${DEPS_DIR}/include
  ${msdfgen_inc}
  ${vinniefalco.url_inc}
  ${LLGL_inc}
  ${skia_inc})

set(my-gui_src
  main.cc
  application.cc
  util/async_task.cc
  util/event_bus.cc
  util/logger.cc
  storage/resource_mgr.cc  
  storage/archive.cc
  storage/xp3_archive.cc
  storage/font_mgr.cc
  render/window/window_mgr.cc
  render/canvas.cc
  media/audio_mgr.cc
  )

# system
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2Mixer REQUIRED)
find_package(Boost REQUIRED COMPONENTS
  thread program_options iostreams locale timer coroutine)
find_package(FFMPEG REQUIRED)
find_package(Freetype REQUIRED)
find_package(Vulkan REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(cairo REQUIRED)

# build
find_package(glm CONFIG REQUIRED)
find_package(rxcpp CONFIG REQUIRED)

set(boost_deps
  Boost::thread
  Boost::program_options
  Boost::iostreams
  Boost::locale
  Boost::timer
  Boost::coroutine)

set(DEPS
  # system
  Threads::Threads
  ${CMAKE_DL_LIBS}
  SDL2::SDL2
  ${SDL2Mixer_LIBRARIES}
  # ${cairo_LIBRARIES}
  ${FFMPEG_LIBRARIES}
  Freetype::Freetype
  Vulkan::Vulkan
  PNG::PNG
  JPEG::JPEG
  ${boost_deps}
  # build
  glm::glm
  rxcpp
  ${LLGL_lib}
  ${msdfgen_lib}
  ${skia_lib}
  )

add_executable(my-gui ${my-gui_src})
target_link_libraries(my-gui
  ${DEPS}
  )
target_include_directories(my-gui PUBLIC ${my-gui_inc})
add_library(my-gui_lib SHARED ${my-gui_src})
target_link_libraries(my-gui_lib
  ${DEPS}
  )
target_include_directories(my-gui_lib PUBLIC ${my-gui_inc})

set(PROJECT_ASSETS_PATH ${PROJECT_SOURCE_DIR}/assets)
set(PROJECT_SHADER_PATH ${PROJECT_ASSETS_PATH}/shaders)
file(GLOB shaders
  ${PROJECT_SHADER_PATH}/*.frag
  ${PROJECT_SHADER_PATH}/*.vert)

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
