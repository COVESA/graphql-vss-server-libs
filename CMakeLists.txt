# Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
#   Author: Alexander Domin (Alexander.Domin@bmw.de)
# Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
#   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a copy of the MPL was
# not distributed with this file, You can obtain one at
# http://mozilla.org/MPL/2.0/.
project(vss-graphql-server-libs)

cmake_minimum_required(VERSION 3.13.0)

set(CMAKE_CXX_STANDARD 17)
set(VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused -Wunused-function -ffunction-sections -fdata-sections ")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--as-needed ")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--as-needed ")

OPTION(
  BUILD_SHARED_LIBS
  "Build shared libraries instead of static."
  OFF
)

OPTION(
  SANITIZE_THREAD
  "Enables Clang thread sanitizer"
  OFF
)

OPTION(
  SANITIZE_ADDRESS
  "Enables Clang address sanitizer"
  OFF
)

###############################################################################
# add COMMONAPI install path to cmake find root path
find_package(PkgConfig)

find_package(Threads REQUIRED)
find_package(Boost 1.71 COMPONENTS system program_options REQUIRED)

if (COMMONAPI_RELEASE)
  if (${COMMONAPI_RELEASE} VERSION_LESS 3.1 OR ${COMMONAPI_RELEASE} VERSION_GREATER 3.2)
    message(WARNING
      "Specified CommonAPI release ${COMMONAPI_RELEASE} not tested! \
       The libraries may fail to compile.")
  endif()
else()
  set(COMMONAPI_RELEASE 3.2)
endif()

find_package(CommonAPI ${COMMONAPI_RELEASE} REQUIRED)
find_package(CommonAPI-SomeIP ${COMMONAPI_RELEASE} REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)

find_package(pegtl 3.2.0 REQUIRED)
find_package(cppgraphqlgen 3.5.0 REQUIRED)
find_package(jwt-cpp)
if (${jwt-cpp_VERSION} VERSION_LESS 0.5.0)
  message(SEND_ERROR "Found jwt-cpp version ${jwt-cpp_VERSION}, but >= 0.5.0 is required.")
endif()

if(DEFINED CMAKE_PREFIX_PATH)
  # The includes of cppgraphqlgen can't be found without this if it is
  # installed outside /usr/local
    foreach(ITEM ${CMAKE_PREFIX_PATH})
      message(STATUS "CMAKE_PREFIX_PATH: " ${ITEM})
      include_directories(SYSTEM "${ITEM}/include")
  endforeach()
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message("Activating DEBUG flags")
  if (SANITIZE_ADDRESS AND SANITIZE_THREAD)
    message(FATAL_ERROR "Address sanitizer not allowed with Thread sanitizer.")
  elseif (SANITIZE_ADDRESS)
    message("Activating ADDRESS sanitizer")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG
        " -fsanitize=address -fno-omit-frame-pointer ")
  elseif (SANITIZE_THREAD)
    message("Activating THREAD sanitizer")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG
        " -fsanitize=thread ")
  endif()
endif()

### Compile Library

add_subdirectory(graphql_vss_server_libs)
