set(thirdparty_path ${PROJECT_SOURCE_DIR}/thirdparty)

option(GLM_TEST_ENABLE "Build unit tests" OFF)
add_subdirectory(glm)

# list(APPEND CMAKE_PREFIX_PATH ${thirdparty_path}/spdlog)
# add_custom_target(third_party)
# include(ExternalProject)

# ExternalProject_Add(RxCpp
#         GIT_REPOSITORY https://github.com/ReactiveX/RxCpp.git
#         GIT_TAG master
#         SOURCE_DIR ${DEPS_DIR}/RxCpp/RxCpp-src
#         BINARY_DIR ${DEPS_DIR}/RxCpp/RxCpp-build
#         CONFIGURE_COMMAND ""
#         )

# add_dependencies(third_party RxCpp)
