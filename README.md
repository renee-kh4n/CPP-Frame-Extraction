# CPP-Frame-Extraction
This project is built using C++ to extract frames (per second) from a video input.


## Setup the build:

Errors with vcpkg and installation. 
How to resolve:
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install opencv:x64-windows

It doesn’t work with the file path “/Renee Khan/” because there is a space
Shorten or move to C:/

.\vcpkg install opencv

.\vcpkg integrate install


Errors with <opencv2/opencv>
You need CMAKE for the linkers
git clone https://github.com/bkaradzic/bgfx.cmake.git
cd bgfx.cmake
git submodule init
git submodule update
cmake -S. -Bcmake-build
cmake --build cmake-build 
Create CMake.txt
cmake_minimum_required(VERSION 3.10)
project(FrameExtractor)


find_package(OpenCV REQUIRED)


add_executable(main main.cpp)
target_link_libraries(main PRIVATE ${OpenCV_LIBS})


mkdir build
cd build


cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/CPP-Frame-Extraction/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .


./main.exe or debug/main.exe (depends where main ends up in T_T)
Alternative: 
cd debug
main.exe
