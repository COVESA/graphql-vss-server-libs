message ("Building tests")

find_package(GTest REQUIRED)

# Build tests
add_executable(test_singletons test_singletons.cpp)
target_link_libraries(
  test_singletons
  graphql_vss_server_libs::graphql_vss_server_libs-support
  GTest::GTest
  GTest::Main
)
gtest_discover_tests(test_singletons)

# Build tests
add_executable(test_scalars test_scalars.cpp)
target_link_libraries(
  test_scalars
  graphql_vss_server_libs::graphql_vss_server_libs-support
  GTest::GTest
  GTest::Main
)
gtest_discover_tests(test_scalars)

# Build tests
add_executable(test_permissions test_permissions.cpp)
target_link_libraries(
  test_permissions
  graphql_vss_server_libs::graphql_vss_server_libs-support
  GTest::GTest
  GTest::Main
)
gtest_discover_tests(test_permissions)
