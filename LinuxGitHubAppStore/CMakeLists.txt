cmake_minimum_required(VERSION 3.16)
project(linux-github-appstore C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4  REQUIRED gtk4)
pkg_check_modules(CURL  REQUIRED libcurl)
pkg_check_modules(JSON  REQUIRED json-c)

include_directories(
    ${GTK4_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

link_directories(
    ${GTK4_LIBRARY_DIRS}
    ${CURL_LIBRARY_DIRS}
    ${JSON_LIBRARY_DIRS}
)

set(SOURCES
    src/main.c
    src/models/app.c
    src/models/release.c
    src/services/github.c
    src/services/settings.c
    src/services/installer.c
    src/services/package_db.c
    src/ui/window.c
    src/ui/sidebar.c
    src/ui/pages/home.c
    src/ui/pages/library.c
    src/ui/pages/settings.c
    src/ui/pages/profile.c
    src/ui/pages/updates.c
)

add_executable(linux-github-appstore ${SOURCES})

target_link_libraries(linux-github-appstore
    ${GTK4_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
)

target_compile_options(linux-github-appstore PRIVATE
    ${GTK4_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
    ${JSON_CFLAGS_OTHER}
    -Wall -O2
)
