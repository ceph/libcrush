cmake_minimum_required(VERSION 2.8.11)

project(libcrush)
set(VERSION 1.0.0)

set(CMAKE_C_FLAGS_NONE "${CMAKE_C_FLAGS_NONE} -Wall -O2 -g")

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

add_custom_target(doc
  COMMAND : \; \( cat doc/Doxygen \; echo 'OUTPUT_DIRECTORY = ${CMAKE_BINARY_DIR}/doc' \) | doxygen -
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

set(crush_srcs
  crush/builder.c
  crush/mapper.c
  crush/crush.c
  crush/hash.c)

add_library(crush STATIC ${crush_srcs})
install(TARGETS crush DESTINATION lib)
install(DIRECTORY crush DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_BINARY_DIR}/crush/acconfig.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/crush/)
install(FILES ${CMAKE_BINARY_DIR}/doc/man/man3/API.3 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man3/libcrush.3)

configure_file(
  ${CMAKE_SOURCE_DIR}/libcrush.pc.in
  ${CMAKE_BINARY_DIR}/libcrush.pc
  @ONLY
)
install(FILES ${CMAKE_BINARY_DIR}/libcrush.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/)

add_subdirectory(test)
add_subdirectory(googletest)
enable_testing()

set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${VERSION})
add_custom_target(dist
  COMMAND git-archive-all/git-archive-all.sh --format tar.gz --prefix ${CMAKE_PROJECT_NAME}-${VERSION}/ ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar.gz
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
