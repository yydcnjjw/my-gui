find_package(GTest CONFIG REQUIRED)

if(GTest_FOUND)
  add_executable(test test.cc)
  target_link_libraries(test
    GTest::gtest_main
    )
endif(GTest_FOUND)
