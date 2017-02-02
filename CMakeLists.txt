cmake_minimum_required(VERSION 2.8.11)

project(libcrush)
set(VERSION 1.0.0)

include_directories(${CMAKE_SOURCE_DIR}/crush)
include_directories(${CMAKE_BINARY_DIR}/crush)

include(CheckIncludeFiles)

CHECK_INCLUDE_FILES("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES("stdint.h" HAVE_STDINT_H)
CHECK_INCLUDE_FILES("linux/types.h" HAVE_LINUX_TYPES_H)

configure_file(
  ${CMAKE_SOURCE_DIR}/crush/config-h.in.cmake
  ${CMAKE_BINARY_DIR}/crush/acconfig.h
)

set(crush_srcs
  crush/builder.c
  crush/mapper.c
  crush/crush.c
  crush/hash.c)

add_library(crush STATIC ${crush_srcs})

add_subdirectory(test)
add_subdirectory(googletest)
enable_testing()

set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${VERSION})
add_custom_target(dist
  COMMAND git-archive-all/git-archive-all.sh --format tar.gz --prefix ${CMAKE_PROJECT_NAME}-${VERSION}/ ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar.gz
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
