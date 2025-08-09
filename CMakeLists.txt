cmake_minimum_required(VERSION 3.16)
project(Interview_Assignment)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost 1.71 REQUIRED COMPONENTS program_options)
find_package(Threads REQUIRED)

# Single public include root for clean include paths like "server/foo.h" or "utils/types.h"
set(PROJECT_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(server_lib STATIC
  src/server/session.cpp
  src/server/session_manager.cpp
)
target_include_directories(server_lib
  PUBLIC 
    ${PROJECT_PUBLIC_INCLUDE_DIR}
)
target_link_libraries(server_lib PUBLIC Boost::headers Threads::Threads)

add_library(client_lib STATIC
  src/client/connection_manager.cpp
)
target_include_directories(client_lib
  PUBLIC 
    ${PROJECT_PUBLIC_INCLUDE_DIR}
)
target_link_libraries(client_lib PUBLIC Boost::headers Threads::Threads)

add_executable(server src/server/server.cpp)
target_link_libraries(server PRIVATE server_lib Boost::program_options)

add_executable(client src/client/client.cpp)
target_link_libraries(client PRIVATE client_lib Boost::program_options)
