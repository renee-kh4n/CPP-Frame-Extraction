#include <opencv2/opencv.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>

// Frame extraction function
void extractFrames(const std::string& videoPath) {
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video file: " << videoPath << std::endl;
        return;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    int frameCount = 0;
    cv::Mat frame;

    while (true) {
        bool success = cap.read(frame);
        if (!success) break;

        if (frameCount % (int)fps == 0) {
            std::ostringstream filename;
            filename << "frame_" << frameCount / (int)fps << ".jpg";
            cv::imwrite(filename.str(), frame);
            std::cout << "Saved: " << filename.str() << std::endl;
        }

        frameCount++;
    }

    cap.release();
}

int main() {
    // Init GLFW
    if (!glfwInit()) return -1;

    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(800, 600, "Frame Extractor GUI", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    char videoPath[256] = "";   // Input buffer
    bool extractNow = false;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Video Frame Extractor");
        ImGui::InputText("Video Path", videoPath, IM_ARRAYSIZE(videoPath));
        if (ImGui::Button("Extract Frames")) {
            extractNow = true;
        }
        ImGui::End();

        // Run extraction outside of ImGui frame
        if (extractNow) {
            extractFrames(videoPath);
            extractNow = false;
        }

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
