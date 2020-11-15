cmake_minimum_required(VERSION 3.5)
file (STRINGS "../VERSION" iceoryx_daemons_VERSION)
project(iceoryx_daemons VERSION ${iceoryx_daemons_VERSION})

add_subdirectory(bouncer)
