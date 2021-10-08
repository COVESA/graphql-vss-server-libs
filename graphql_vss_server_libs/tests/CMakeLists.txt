# Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG), Author: Alexander Domin (Alexander.Domin@bmw.de)
# Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA, Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a copy of the MPL was
# not distributed with this file, You can obtain one at
# http://mozilla.org/MPL/2.0/.

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
