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