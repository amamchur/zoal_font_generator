cmake_minimum_required(VERSION 3.19)
project(zoal_font_generator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Boost 1.75 COMPONENTS program_options REQUIRED)
find_package(Freetype REQUIRED)

include_directories(/usr/local/include $ENV{ZOAL_PATH})

add_executable(GenFont main.cpp font_generator.cpp)
add_executable(CheckFont check_font.cpp roboto_regular_16.cpp)

add_executable(gui gui.cpp
        oledscreen.h
        mainwindow.ui
        oledscreen.cpp
        mainwindow.cpp
        font_generator.cpp
        mainwindow.h)
target_link_libraries(gui PRIVATE Qt5::Widgets ${FREETYPE_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(gui PRIVATE ${FREETYPE_INCLUDE_DIRS})

target_link_libraries(GenFont ${FREETYPE_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(GenFont PRIVATE ${FREETYPE_INCLUDE_DIRS})
