enable_testing()

set(UNITTEST_CXX_FLAGS "-I${CMAKE_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/googletest/googlemock/include -I${CMAKE_BINARY_DIR}/googletest/googlemock/include -I${CMAKE_SOURCE_DIR}/googletest/googletest/include -I${CMAKE_BINARY_DIR}/googletest/googletest/include -fno-strict-aliasing --std=c++11")

add_executable(unittest_builder test_builder.cc)
set_target_properties(unittest_builder PROPERTIES COMPILE_FLAGS ${UNITTEST_CXX_FLAGS})
target_link_libraries(unittest_builder crush gtest gtest_main)
add_test(builder unittest_builder)

add_executable(unittest_helpers test_helpers.cc)
set_target_properties(unittest_helpers PROPERTIES COMPILE_FLAGS ${UNITTEST_CXX_FLAGS})
target_link_libraries(unittest_helpers crush gtest gtest_main)
add_test(helpers unittest_helpers)

add_executable(unittest_mapper test_mapper.cc)
set_target_properties(unittest_mapper PROPERTIES COMPILE_FLAGS ${UNITTEST_CXX_FLAGS})
target_link_libraries(unittest_mapper crush gtest gtest_main)
add_test(mapper unittest_mapper)
