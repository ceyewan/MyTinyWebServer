cmake_minimum_required(VERSION 3.22)
project(WebServer)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(WebServer
        ./src/buffer.cpp
        ./src/epoller.cpp
        ./src/httpconn.cpp
        ./src/httprequest.cpp
        ./src/httpresponse.cpp
        ./src/main.cpp
        ./src/sqlconnRAII.cpp
        ./src/timer.cpp
        ./src/webserver.cpp)

target_link_libraries(WebServer mysqlclient)

