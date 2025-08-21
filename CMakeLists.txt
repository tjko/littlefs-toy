# CMakeLists.txt for LittleFS-Toy
#

cmake_minimum_required(VERSION 3.20)


project(littlefs-toy
	VERSION 1.0.0
	DESCRIPTION "littlefs-toy - A tool for creating and modifying LittleFS images."
	HOMEPAGE_URL https://github.com/tjko/littlefs-toy/
	LANGUAGES C
	)

set(CMAKE_C_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(BUILD_TAG beta CACHE STRING "Build Tag")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, e.g. Debug, Release")
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Release)
endif()


message("==========================================================")
message("        BUILD_TAG: ${BUILD_TAG}")
message(" CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("==========================================================")

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(getopt.h HAVE_GETOPT_H)

check_function_exists(getopt_long HAVE_GETOPT_LONG)


# LittleFS

add_library(littlefs INTERFACE)
target_include_directories(littlefs INTERFACE
  libs/littlefs/
)
target_sources(littlefs INTERFACE
  libs/littlefs/lfs.c
  libs/littlefs/lfs_util.c
)
set_source_files_properties(
  libs/littlefs/lfs.c
  PROPERTIES
  COMPILE_OPTIONS "-Wno-unused-function"
)


add_executable(lfs
  src/lfs.c
  src/lfs_driver.c
  src/lfs_extra.c
  src/util.c
)
target_include_directories(lfs PRIVATE src)
configure_file(src/config.h.in config.h)

if (HAVE_GETOPT_LONG)
  message("Using getopt_long from the system.")
else()
  message("Using included getopt_long.")
  target_sources(lfs PRIVATE
    src/getopt/getopt.c
    src/getopt/getopt1.c
  )
endif()


target_link_libraries(lfs PRIVATE
  littlefs
)


target_compile_options(lfs PRIVATE
  -Wall
  -Wextra
  -fstack-protector-all
  -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
)

target_compile_definitions(lfs PRIVATE
  HAVE_CONFIG_H
  LFS_DEFINES=lfs_opts.h
  LFS_THREADSAFE
  LFS_NO_DEBUG
  LFS_NO_WARN
#  LFS_NO_ERROR
)


install(TARGETS lfs)
install(FILES man/lfs.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES README.md LICENSE TYPE DOC)

