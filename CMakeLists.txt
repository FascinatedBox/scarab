project(scarab)
cmake_minimum_required(VERSION 3.5.0)

include(FindPkgConfig)
find_package(PNG REQUIRED)
pkg_check_modules(
    XCB_LIBS
        xcb
        xcb-image
    REQUIRED
)

if(WITH_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0")
endif()

add_executable(
    scarab
        src/scarab.c
        src/xpick.c
)

target_link_libraries(
    scarab
        png
        xcb
        xcb-image
)

install(
    TARGETS
        scarab
    DESTINATION
        bin
)
