enable_testing()

set(UNITTEST_CXX_FLAGS "-I${CMAKE_SOURCE_DIR}/googletest/googlemock/include -I${CMAKE_BINARY_DIR}/googletest/googlemock/include -I${CMAKE_SOURCE_DIR}/googletest/googletest/include -I${CMAKE_BINARY_DIR}/googletest/googletest/include -fno-strict-aliasing --std=c++11")

add_executable(unittest_builder test_builder.cc)
set_target_properties(unittest_builder PROPERTIES COMPILE_FLAGS ${UNITTEST_CXX_FLAGS})
target_link_libraries(unittest_builder crush gtest gtest_main)
add_test(builder unittest_builder)
