# Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
#   Author: Alexander Domin (Alexander.Domin@bmw.de)
# Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA, Author:
#   Gabriel Fernandes (g7fernandes@profusion.mobi)
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a copy of the MPL was
# not distributed with this file, You can obtain one at
# http://mozilla.org/MPL/2.0/.

set(SUPPORT_SRC
  debug.cpp # keep first
  commonapi-singletons.cpp
  log.cpp
  permissions.cpp
  scalars.cpp
  singleton.cpp
)

add_library(graphql_vss_server_libs-support ${SUPPORT_SRC})
add_library(graphql_vss_server_libs::graphql_vss_server_libs-support ALIAS graphql_vss_server_libs-support)
set_target_properties(
  graphql_vss_server_libs-support
  PROPERTIES
  VERSION ${GRAPHQL_VSS_SERVER_LIBS_VERSION}
  SOVERSION ${GRAPHQL_VSS_SERVER_LIBS_SOVERSION}
)
target_include_directories(
  graphql_vss_server_libs-support
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
  "$<BUILD_INTERFACE:${COMMONAPI_INCLUDE_DIRS}>"
  "$<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>"
  "$<BUILD_INTERFACE:${DLT_INCLUDE_DIRS}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(
  graphql_vss_server_libs-support
  CommonAPI-SomeIP
  cppgraphqlgen::graphqlresponse
  ${DLT_LIBRARIES}
)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(EXTRA_EXPORT_HEADER "
#define GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG 1
#define GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG_LOCKS 1
#define GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG_COLORS 1
")
endif()
generate_export_header(
  graphql_vss_server_libs-support
  CUSTOM_CONTENT_FROM_VARIABLE EXTRA_EXPORT_HEADER
)

install(
  FILES
    commonapi-singletons.hpp
    debug.hpp
    demangle.hpp
    log.hpp
    permissions.hpp
    scalars.hpp
    singleton.hpp
    spinlock.hpp
    type_traits_extras.hpp
    dlt_helpers.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/graphql_vss_server_libs-support_export.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphql_vss_server_libs/support
)
