cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(FrameExtractor)

find_package(OpenCV REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE ${OpenCV_LIBS})
