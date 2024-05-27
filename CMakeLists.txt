cmake_minimum_required(VERSION 3.10)

# Project name
project(BookScraper)

# Set C standard
set(CMAKE_C_STANDARD 99)

# Integrate vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# Find libcurl package
find_package(CURL REQUIRED)

# Find libxml2 package
find_package(LibXml2 REQUIRED)

# Include directories
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${LIBXML2_INCLUDE_DIRS})

# Source files
set(SOURCES src/main.c)

# Add executable
add_executable(BookScraper ${SOURCES})

# Link libraries
target_link_libraries(BookScraper ${CURL_LIBRARIES} ${LIBXML2_LIBRARIES})

