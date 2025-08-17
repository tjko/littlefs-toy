# CMakeLists.txt for LittleFS-Toy
#

cmake_minimum_required(VERSION 3.20)


project(littlefs-toy
	VERSION 0.1.0
	DESCRIPTION "littlefs-toy - A tool for creating and modifying LittleFS images."
	HOMEPAGE_URL https://github.com/tjko/littlefs-toy/
	LANGUAGES C
	)

set(CMAKE_C_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(BUILD_TAG beta CACHE STRING "Build Tag")


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



add_executable(lfs
  src/lfs.c
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
  -fstack-protector-all
  -DHAVE_CONFIG_H
)

target_compile_definitions(lfs PRIVATE
  LFS_THREADSAFE=1
  LFS_NO_DEBUG=0
)


install(TARGETS lfs)
install(FILES lfs.1 TYPE MAN)
install(FILES README.md LICENSE TYPE DOC)

