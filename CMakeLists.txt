# CMakeLists.txt for LittleFS-Toy
#

cmake_minimum_required(VERSION 3.20)



set(CMAKE_C_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(BUILD_TAG beta)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, e.g. Debug, Release")
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Release)
endif()


project(littlefs-toy
	VERSION 1.1.1
	DESCRIPTION "littlefs-toy - A tool for creating and modifying LittleFS images."
	HOMEPAGE_URL https://github.com/tjko/littlefs-toy/
	LANGUAGES C
	)



include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(sys/errno.h HAVE_SYS_ERRNO_H)

check_function_exists(getopt_long HAVE_GETOPT_LONG)

find_package(Python3 COMPONENTS Interpreter Development)


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


add_executable(lfst
  src/lfst.c
  src/lfs_driver.c
  src/lfs_extra.c
  src/util.c
)
target_include_directories(lfst PRIVATE src)
configure_file(src/config.h.in config.h)

if (HAVE_GETOPT_LONG)
  message("Using getopt_long from the system.")
else()
  message("Using included getopt_long.")
  target_sources(lfst PRIVATE
    src/getopt/getopt.c
    src/getopt/getopt1.c
  )
endif()


if (MINGW)
   message("Building with mingw-w64")
   target_link_libraries(lfst -static gcc winpthread littlefs -dynamic)
else()
   target_link_libraries(lfst PRIVATE littlefs)
endif()



target_compile_options(lfst PRIVATE
  -Wall
  -Wextra
  -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
)
if (NOT MINGW)
  target_compile_options(lfst PRIVATE
    -fstack-protector-all
  )
endif()

target_compile_definitions(lfst PRIVATE
  HAVE_CONFIG_H
  LFS_DEFINES=lfs_opts.h
  LFS_THREADSAFE
  LFS_NO_DEBUG
  LFS_NO_WARN
#  LFS_NO_ERROR
)


install(TARGETS lfst)
install(FILES man/lfst.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES README.md LICENSE TYPE DOC)


if (Python3_FOUND)
  enable_testing()
  add_test(NAME lfs_tests
	 COMMAND ${Python3_EXECUTABLE} test_lfst.py
	 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
	 )

  set_property(TEST lfs_tests PROPERTY ENVIRONMENT "LFS=$<TARGET_FILE:lfst>" "DEBUG=1")
endif()


message("==========================================================")
message("              BUILD_TAG: ${BUILD_TAG}")
message("       CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("      CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message("   CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
message(" CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message("   CMAKE_CROSSCOMPILING: ${CMAKE_CROSSCOMPILING}")
message("==========================================================")

