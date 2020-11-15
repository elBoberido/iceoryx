cmake_minimum_required(VERSION 3.5)
project(iceoryx_bouncer VERSION ${iceoryx_daemons_VERSION})

find_package(iceoryx_utils REQUIRED)

add_executable(
    iox-bouncer
    source/main.cpp
)

target_link_libraries(
    iox-bouncer
    PRIVATE
    iceoryx_utils::iceoryx_utils
)
