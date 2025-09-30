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

.\vcpkg install opencv

.\vcpkg integrate install

//It doesn’t work with the file path “/Renee Khan/” because there is a space
Shorten or move to C:/


Errors with <opencv2/opencv>
You need CMAKE for the linkers
git clone https://github.com/bkaradzic/bgfx.cmake.git
cd bgfx.cmake
git submodule init
git submodule update


git clone https://github.com/ocornut/imgui.git

git clone https://github.com/glfw/glfw.git
cd glfw
cmake -S . -B build
cmake --build build



Create CMake.txt
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(FrameExtractor)

find_package(OpenCV REQUIRED)

find_package(OpenGL REQUIRED)

add_subdirectory(glfw)


# --- ImGui sources ---
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# --- Add executable ---
add_executable(main
    main.cpp
    ${IMGUI_SOURCES}
)

# --- Include dirs ---
target_include_directories(main PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# --- Link libs ---
target_link_libraries(main PRIVATE
    ${OpenCV_LIBS}
    glfw
    OpenGL::GL
)


mkdir build
cd build


cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/CPP-Frame-Extraction/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .


./main.exe or debug/main.exe (depends where main ends up in T_T)
Alternative: 
cd debug
main.exe
