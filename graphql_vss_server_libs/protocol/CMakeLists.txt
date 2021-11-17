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

find_package(OpenSSL REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(websocketpp 0.8.2 REQUIRED)

set(PROTOCOL_SRC
  exceptions.cpp
  graphqlconnection.cpp
  graphqlconnectionoperation.cpp
  graphqlrequesthandlers.cpp
  graphqlrequeststate.cpp
  graphqlserver.cpp
  jwtauthorizer.cpp
  dummyauthorizer.cpp
)

if(DISABLE_PERMISSIONS)
  message(WARNING "Disabling permissions, all requests will be allowed!")
  set(PERMISSIONS_FLAGS "DISABLE_PERMISSIONS=true")
else()
  set(PERMISSIONS_FLAGS "DISABLE_PERMISSIONS=false")
endif()

add_library(graphql_vss_server_libs-protocol ${PROTOCOL_SRC})
add_library(graphql_vss_server_libs::graphql_vss_server_libs-protocol ALIAS graphql_vss_server_libs-protocol)
set_target_properties(
  graphql_vss_server_libs-protocol
  PROPERTIES
  VERSION ${GRAPHQL_VSS_SERVER_LIBS_VERSION}
  SOVERSION ${GRAPHQL_VSS_SERVER_LIBS_SOVERSION}
)
target_link_libraries(
  graphql_vss_server_libs-protocol
  PUBLIC
  graphql_vss_server_libs::graphql_vss_server_libs-support
  cppgraphqlgen::graphqlservice
  cppgraphqlgen::graphqljson
  cppgraphqlgen::graphqlresponse
  Boost::system
)
target_link_libraries(
  graphql_vss_server_libs-protocol
  PRIVATE
  OpenSSL::Crypto
)
target_include_directories(
  graphql_vss_server_libs-protocol
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  ${WEBSOCKETPP_INCLUDE_DIR}
)
target_compile_definitions(
  graphql_vss_server_libs-protocol
  PRIVATE
  ${PERMISSIONS_FLAGS}
)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(EXTRA_EXPORT_HEADER "#define GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG 1")
endif()
generate_export_header(
  graphql_vss_server_libs-protocol
  CUSTOM_CONTENT_FROM_VARIABLE EXTRA_EXPORT_HEADER
)

install(
  FILES
    authorizer.hpp
    exceptions.hpp
    graphqlconnection.hpp
    graphqlconnectionoperation.hpp
    graphqlrequesthandlers.hpp
    graphqlrequeststate.hpp
    graphqlserver.hpp
    jwtauthorizer.hpp
    dummyauthorizer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/graphql_vss_server_libs-protocol_export.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphql_vss_server_libs/protocol
)
