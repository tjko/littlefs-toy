# CMakeLists.txt for LittleFS-Toy
#

cmake_minimum_required(VERSION 3.18)


project(littlefs-toy
	VERSION 0.1.0
	DESCRIPTION "littlefs-toy - A tool for creating and modifying LittleFS images."
	HOMEPAGE_URL https://github.com/tjko/littlefs-toy/
	LANGUAGES C
	)

set(CMAKE_C_STANDARD 11)

include(GNUInstallDirs)


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
)

target_include_directories(lfs PRIVATE src)

target_link_libraries(lfs PRIVATE
  littlefs
)
  

install(TARGETS lfs)
install(FILES lfs.1 TYPE MAN)
install(FILES README.md LICENSE TYPE DOC)

