macro(build_dep dep_name)
  set(dep_name ${dep_name})
  set(dep_src_dir ${DEPS_DIR}/${dep_name}/${dep_name}-src)
  set(dep_build_dir ${DEPS_DIR}/${dep_name}/${dep_name}-build)
  message("build dep: ${dep_name}")
  message("src dir ${dep_src_dir}")
  message("build dir ${dep_build_dir}")
  
  configure_file(CMakeLists.txt.${dep_name}.in
    ${dep_name}/CMakeLists.txt @ONLY)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY ${DEPS_DIR}/${dep_name})
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY ${DEPS_DIR}/${dep_name})

  set(${dep_name}_src_dir ${dep_src_dir})
  set(${dep_name}_build_dir ${dep_build_dir})
endmacro()

# googtest
build_dep(googletest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(gtest_dir ${DEPS_DIR}/lib64/cmake/GTest)
if(EXISTS ${gtest_dir})
  set(GTest_DIR ${DEPS_DIR}/lib64/cmake/GTest CACHE INTERNAL "")  
else()
  set(GTest_DIR ${DEPS_DIR}/lib/cmake/GTest CACHE INTERNAL "")  
endif()

# # rxcpp
build_dep(rxcpp)
set(rxcpp_DIR ${DEPS_DIR}/share/rxcpp/cmake CACHE INTERNAL "")

# # glm
build_dep(glm)
set(glm_DIR ${glm_src_dir}/cmake/glm CACHE INTERNAL "")

# msdfgen
build_dep(msdfgen)
set(msdfgen_lib ${msdfgen_build_dir}/libmsdfgen.a CACHE INTERNAL "")
set(msdfgen_inc ${msdfgen_src_dir} CACHE INTERNAL "")

# vinniefalco/url
build_dep(vinniefalco.url)
set(vinniefalco.url_inc ${vinniefalco.url_src_dir}/include CACHE INTERNAL "")

# LLGL
build_dep(LLGL)
file(GLOB LLGL_lib ${LLGL_build_dir}/build/*)
set(LLGL_inc ${LLGL_src_dir}/include CACHE INTERNAL "")
set(LLGL_lib ${LLGL_lib} CACHE INTERNAL "")

# skia
build_dep(skia)
set(skia_inc
  ${DEPS_DIR}/include/skia
  CACHE INTERNAL "")
set(skia_lib ${skia_src_dir}/out/Release/libskia.a CACHE INTERNAL "")

build_dep(oiio)
set(OpenImageIO_DIR ${DEPS_DIR}/lib/cmake/OpenImageIO CACHE INTERNAL "")

