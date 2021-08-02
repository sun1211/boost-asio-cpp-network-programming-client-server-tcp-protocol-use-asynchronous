project(server)
cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

if(CMAKE_CXX_STANDARD EQUAL 98 OR CMAKE_CXX_STANDARD LESS 14)
   message(FATAL_ERROR "app requires c++14 or newer")
elseif(NOT CMAKE_CXX_STANDARD)
   set(CMAKE_CXX_STANDARD 14)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_package(Boost 1.60 REQUIRED COMPONENTS thread system chrono)

add_executable(server server.cpp)

target_link_libraries(server Boost::thread Boost::system Boost::chrono)