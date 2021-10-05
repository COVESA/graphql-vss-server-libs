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

find_package(CommonAPI 3.2 REQUIRED)
find_package(CommonAPI-SomeIP 3.2 REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)

find_package(pegtl 3.2.0 REQUIRED)
find_package(cppgraphqlgen 3.5.0 REQUIRED)

if(DEFINED CMAKE_PREFIX_PATH)
	# The includes of cppgraphqlgen can't be found without this if it is
  # installed outside /usr/local
  	foreach(ITEM ${CMAKE_PREFIX_PATH})
	  	message(STATUS "CMAKE_PREFIX_PATH: " ${ITEM})
	  	include_directories(SYSTEM "${ITEM}/include")
	endforeach()
endif()

### Compile Library

add_subdirectory(vss_cpp_graphql)
