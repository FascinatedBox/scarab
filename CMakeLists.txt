project(scarab)
cmake_minimum_required(VERSION 3.5.0)

if(WITH_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0")
endif()

find_package(PNG)
find_package(X11)

add_executable(
    scarab
        src/scarab.c
)

target_link_libraries(
    scarab
        png
        X11
)

install(
    TARGETS
        scarab
    DESTINATION
        bin
)
