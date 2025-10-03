cmake_minimum_required(VERSION 3.25)
project(KnotesCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find libgit2
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

include_directories(${LIBGIT2_INCLUDE_DIRS})
link_directories(${LIBGIT2_LIBRARY_DIRS})

# Add src subdirectory
add_subdirectory(src)

