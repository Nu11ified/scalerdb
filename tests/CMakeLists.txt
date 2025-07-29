add_executable(scalerdb_tests test_main.cpp)

target_link_libraries(scalerdb_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(scalerdb_tests) 