# top dir cmake project for libhydrasdr + hydrasdr-tools

cmake_minimum_required(VERSION 3.10)
project(hydrasdr_all LANGUAGES C HOMEPAGE_URL https://hydrasdr.com/)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#provide missing strtoull() for VC11
if(MSVC11)
  add_definitions(-Dstrtoull=_strtoui64)
endif(MSVC11)

add_subdirectory(libhydrasdr)
add_subdirectory(hydrasdr-tools)

########################################################################
# Create uninstall target
########################################################################

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
  COMMENT "Provide uninstall target"
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Copy Files mingw64 only
########################################################################

if(WIN32 AND MINGW)
  message("copy libusb-1.0.dll")
  configure_file(
    C:\\msys64\\mingw64\\bin\\libusb-1.0.dll
    ${CMAKE_CURRENT_BINARY_DIR}/hydrasdr-tools/src/libusb-1.0.dll
    COPYONLY)

  message("copy libwinpthread-1.dll")
  configure_file(
    C:\\msys64\\mingw64\\bin\\libwinpthread-1.dll
    ${CMAKE_CURRENT_BINARY_DIR}/hydrasdr-tools/src/libwinpthread-1.dll
    COPYONLY)
endif()
