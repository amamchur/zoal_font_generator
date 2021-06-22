cmake_minimum_required(VERSION 3.19)
project(zoal_font_generator)

find_package(Boost 1.75 COMPONENTS program_options json REQUIRED)
find_package(Freetype REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_executable(GenFont main.cpp)
add_executable(CheckFont check_font.cpp roboto_regular_16.cpp)

target_link_libraries(GenFont ${FREETYPE_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(GenFont PRIVATE ${FREETYPE_INCLUDE_DIRS})
