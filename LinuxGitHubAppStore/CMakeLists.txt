cmake_minimum_required(VERSION 3.16)
project(linux-github-appstore C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED json-c)

# Include directories
include_directories(${GTK4_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${JSON_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link directories
link_directories(${GTK4_LIBRARY_DIRS})
link_directories(${CURL_LIBRARY_DIRS})
link_directories(${JSON_LIBRARY_DIRS})

# Source files
set(SOURCES
    src/main.c
    src/models/app.c
    src/services/github.c
    src/services/settings.c
    src/ui/window.c
    src/ui/sidebar.c
    src/ui/pages/home.c
    src/ui/pages/library.c
    src/ui/pages/settings.c
    src/ui/pages/profile.c
)

# Create executable
add_executable(linux-github-appstore ${SOURCES})

# Link libraries
target_link_libraries(linux-github-appstore
    ${GTK4_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
)

# Compiler flags
target_compile_options(linux-github-appstore PRIVATE
    ${GTK4_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
    ${JSON_CFLAGS_OTHER}
    -Wall -Wextra -O2
)
