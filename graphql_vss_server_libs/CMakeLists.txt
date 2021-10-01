include(GNUInstallDirs)
include(GenerateExportHeader)

set(GRAPHQL_VSS_SERVER_LIBS_SOVERSION 0)
set(GRAPHQL_VSS_SERVER_LIBS_VERSION 0.1.0)

add_subdirectory(support)
add_subdirectory(protocol)

if(BUILD_TESTS)
  message(STATUS "Including tests/")
  add_subdirectory(tests)
endif()

install(
  TARGETS
    graphql_vss_server_libs-support
    graphql_vss_server_libs-protocol
  EXPORT graphql_vss_server_libs-targets
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/graphql_vss_server_libs-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/graphql_vss_server_libs-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/graphql_vss_server_libs
)
write_basic_package_version_file(
  graphql_vss_server_libs-config-version.cmake
  VERSION ${GRAPHQL_VSS_SERVER_LIBS_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/graphql_vss_server_libs-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/graphql_vss_server_libs-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/graphql_vss_server_libs
)
install(EXPORT graphql_vss_server_libs-targets
  NAMESPACE graphql_vss_server_libs::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/graphql_vss_server_libs
)
