cmake_minimum_required(VERSION 3.14)

project(application_proxy LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------ Qt paketi ------------------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network)

# ------------------ Executable ------------------
add_executable(application_proxy
    main.cpp
    server.h
    server.cpp
    proxy.h proxy.cpp
    client.h client.cpp
)

# ------------------ Link libraries ------------------
target_link_libraries(application_proxy
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# ------------------ Instalacija ------------------
include(GNUInstallDirs)
install(TARGETS application_proxy
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
