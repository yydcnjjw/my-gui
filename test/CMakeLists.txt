find_package(GTest CONFIG REQUIRED)

if(GTest_FOUND)
  add_executable(test
    test.cc
    render/node_test.cc
    )
  target_link_libraries(test
    GTest::gtest_main
    my-gui_lib
    )
endif(GTest_FOUND)
