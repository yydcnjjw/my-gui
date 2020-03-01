cmake_minimum_required(VERSION 3.11)
project(my-gui)

set(CMAKE_CXX_STANDARD 17)

set(DEPLOY_DIR ${PROJECT_BINARY_DIR}/deploy)
set(DEPS_DIR ${PROJECT_BINARY_DIR}/deps)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DEPLOY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DEPLOY_DIR}/lib)

set(CMAKE_PREFIX_PATH ${PROJECT_BINARY_DIR})

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(test)

