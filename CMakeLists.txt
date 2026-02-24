cmake_minimum_required(VERSION 3.16)

project(file-manager-server-webRtcp LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED CONFIG)
find_package(libpqxx REQUIRED)
find_package(jwt-cpp REQUIRED CONFIG)

add_library(file-manager-server-lib
  server.h server.cpp
  net.h
  session.h session.cpp
  settings.h
)
target_link_libraries(file-manager-server-lib PUBLIC
  Boost::boost
  OpenSSL::SSL
  nlohmann_json::nlohmann_json
  libpqxx::pqxx)



add_executable(file-manager-server
  main_file.cpp
)
target_link_libraries(file-manager-server file-manager-server-lib)



