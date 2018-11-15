cmake_minimum_required(VERSION 3.12)
project(multi_threaded_web_server)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED system regex date_time thread)

include_directories(${Boost_INCLUDE_DIR})

file(GLOB server_src "src/*.h" "src/*.cpp")

add_executable(multi_threaded_web_server ${server_src})

TARGET_LINK_LIBRARIES(multi_threaded_web_server ${Boost_LIBRARIES})
