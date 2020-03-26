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
set(GTest_DIR ${DEPS_DIR}/lib/cmake/GTest CACHE INTERNAL "")

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
