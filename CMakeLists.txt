cmake_minimum_required(VERSION 3.5)

project(ChatServer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -g
    -O0
    -flto
    -Wall
    -Wextra
    -Wpedantic
    -Wcast-align
    -Wcast-qual
    -Wconversion
    -Wctor-dtor-privacy
    -Wenum-compare
    -Wfloat-equal
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
)

set(BUILD_SHARED_LIBS OFF)

find_package(QT NAMES Qt5 COMPONENTS Core Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)

add_executable(ChatServer
  main.cpp
  src/chatServer.h src/chatServer.cpp
  src/usersmanager.h src/usersmanager.cpp
  src/tcpsocket.h src/tcpsocket.cpp
  src/user.h src/user.cpp
  src/room.h src/room.cpp
)
target_link_libraries(ChatServer Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(ChatServer Qt${QT_VERSION_MAJOR}::Network)
