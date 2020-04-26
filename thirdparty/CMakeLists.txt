# googtest
configure_file(CMakeLists.txt.googletest.in
               googletest/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${DEPS_DIR}/googletest)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${DEPS_DIR}/googletest)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set(gtest_dir ${DEPS_DIR}/lib64/cmake/GTest)
if(EXISTS ${gtest_dir})
  set(GTest_DIR ${DEPS_DIR}/lib64/cmake/GTest CACHE INTERNAL "")  
else()
  set(GTest_DIR ${DEPS_DIR}/lib/cmake/GTest CACHE INTERNAL "")  
endif()

# rxcpp
configure_file(CMakeLists.txt.rxcpp.in
               rxcpp/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${DEPS_DIR}/rxcpp)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${DEPS_DIR}/rxcpp)
set(rxcpp_DIR ${DEPS_DIR}/share/rxcpp/cmake CACHE INTERNAL "")

# glm
configure_file(CMakeLists.txt.glm.in
               glm/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${DEPS_DIR}/glm)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${DEPS_DIR}/glm)
set(glm_DIR ${DEPS_DIR}/glm/glm-src/cmake/glm CACHE INTERNAL "")

# msdfgen
configure_file(CMakeLists.txt.msdfgen.in
               msdfgen/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${DEPS_DIR}/msdfgen)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${DEPS_DIR}/msdfgen)
set(msdfgen_lib ${DEPS_DIR}/msdfgen/msdfgen-build/libmsdfgen.a CACHE INTERNAL "")
set(msdfgen_inc ${DEPS_DIR}/msdfgen/msdfgen-src CACHE INTERNAL "")

# vinniefalco/url
configure_file(CMakeLists.txt.vinniefalco.url.in
               vinniefalco-url/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${DEPS_DIR}/vinniefalco-url)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${DEPS_DIR}/vinniefalco-url)
set(vinniefalco.url_inc ${DEPS_DIR}/vinniefalco-url/vinniefalco-url-src/include CACHE INTERNAL "")

# LLGL
configure_file(CMakeLists.txt.LLGL.in
               LLGL/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${DEPS_DIR}/LLGL)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${DEPS_DIR}/LLGL)
file(GLOB LLGL_lib ${DEPS_DIR}/LLGL/LLGL-build/build/*)
set(LLGL_inc ${DEPS_DIR}/LLGL/LLGL-src/include CACHE INTERNAL "")
set(LLGL_lib ${LLGL_lib} CACHE INTERNAL "")

# list(APPEND CMAKE_MODULE_PATH ${DEPS_DIR}/LLGL/LLGL-src/cmake)
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE INTERNAL "" FORCE)
# netlib.url
# configure_file(CMakeLists.txt.netlib.url.in
#                netlib.url/CMakeLists.txt)
# execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
#   WORKING_DIRECTORY ${DEPS_DIR}/netlib.url)
# execute_process(COMMAND ${CMAKE_COMMAND} --build .
#   WORKING_DIRECTORY ${DEPS_DIR}/netlib.url)
# set(netlib.url_DIR ${DEPS_DIR}/netlib.url/netlib.url-src/cmake CACHE INTERNAL "")
