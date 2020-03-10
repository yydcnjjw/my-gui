# find_package(GTest REQUIRED)
# enable_testing()

# add_test()
add_executable(test test.cc)
target_link_libraries(test gtest_main)
