#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <atomic>
#include <chrono>
#include <cstdlib>


#include <algorithm>
#define NOMINMAX
#include <windows.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <commdlg.h>

namespace fs = std::filesystem;

// ======================
// Thread-safe queue
// ======================
template <typename T>
class SafeQueue {
    std::queue<T> q;
    std::mutex m;
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m);
        q.push(std::move(value));
    }

    bool pop(T& value) {
        std::lock_guard<std::mutex> lock(m);
        if (q.empty()) return false;
        value = std::move(q.front());
        q.pop();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m);
        return q.empty();
    }
};

// ======================
// File dialog helper
// ======================
std::string openFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Video Files\0*.mp4;*.avi;*.mkv;*.mov\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    ofn.lpstrTitle = "Select Video File";
    if (GetOpenFileName(&ofn)) {
        return std::string(filename);
    }
    return "";
}

// ======================
// Fix OpenGL defines for Windows
// ======================
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

GLuint matToTexture(const cv::Mat& mat) {
    if (mat.empty()) return 0;
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (mat.channels() == 3) ? GL_BGR : GL_BGRA;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mat.cols, mat.rows, 0, format, GL_UNSIGNED_BYTE, mat.ptr());
    return textureID;
}

// ======================
// Main Program
// ======================
int main() {
    if (!glfwInit()) return -1;
    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Multithreaded Frame Extractor", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    // ======================
    // State variables
    // ======================
    std::string videoPath;
    std::string outputDir;
    std::string buildDir = fs::current_path().string();  // typically /build/Debug
    std::string projectRoot = fs::absolute(buildDir + "/../..").string(); // go one directory up
    std::string pythonScript = (fs::path(projectRoot) / "remove_bg.py").string();

    fs::path outputRoot = fs::path(buildDir);
    fs::create_directories(outputRoot);

    cv::VideoCapture cap;
    SafeQueue<std::pair<int, cv::Mat>> frameQueue;
    std::atomic<bool> extracting = false;
    std::atomic<bool> extractionDone = false;
    std::atomic<int> savedCount = 0;
    std::atomic<int> totalFrames = 0;
    std::atomic<double> fps = 0;
    GLuint textureID = 0;

    // ======================
    // GUI loop
    // ======================
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Video Frame Extractor", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (ImGui::Button("Choose Video File")) {
            std::string chosen = openFileDialog();
            if (!chosen.empty()) {
                videoPath = chosen;
                outputDir = (outputRoot / fs::path(videoPath).stem()).string();
                fs::create_directories(outputDir);
                cap.open(videoPath);

                if (cap.isOpened()) {
                    fps = cap.get(cv::CAP_PROP_FPS);
                    totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
                }
                else {
                    std::cerr << "Error: Cannot open video.\n";
                }
            }
        }

        if (!videoPath.empty()) {
            ImGui::Text("Video: %s", videoPath.c_str());
            ImGui::Text("FPS: %.2f", fps.load());
            ImGui::Text("Total Frames: %d", totalFrames.load());
            ImGui::Text("Output Directory: %s", outputDir.c_str());
        }

        static int savedFPS = 1;
        ImGui::Text("Frames per second to save:");
        for (int i = 1; i <= 5; i++) {
            char label[2]; snprintf(label, sizeof(label), "%d", i);
            if (ImGui::RadioButton(label, savedFPS == i)) savedFPS = i;
            ImGui::SameLine();
        }
        ImGui::NewLine();

        if (!extracting && ImGui::Button("Start Extraction")) {
            if (cap.isOpened()) {
                extracting = true;
                extractionDone = false;
                savedCount = 0;

                // --- Thread 1: Frame extraction ---
                std::thread([&]() {
                    cv::Mat frame;
                    int frameIndex = 0, savedIndex = 0;
                    int step = std::max(1, static_cast<int>(fps / savedFPS));

                    while (cap.read(frame)) {
                        if (frameIndex % step == 0) {
                            frameQueue.push({ savedIndex++, std::move(frame) });
                        }
                        frameIndex++;
                    }

                    extractionDone = true;
                    cap.release();
                    }).detach();


                // --- Thread 2: Frame saving ---
                std::thread([&]() {
                    while (!extractionDone || !frameQueue.empty()) {
                        std::pair<int, cv::Mat> item;
                        if (frameQueue.pop(item)) {
                            std::string filename = (fs::path(outputDir) / ("frame_" + std::to_string(item.first) + ".png")).string();
                            cv::imwrite(filename, item.second);
                            savedCount++;
                        }
                        else {
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        }
                    }

                    // --- After saving all frames, run rembg batch ---

                    std::string command = "python \"" + pythonScript + "\" \"" + outputDir + "\"";
                    std::cout << "Running: " << command << std::endl;
                    system(command.c_str());

                    extracting = false;
                    }).detach();
            }
        }

        if (extracting) {
            ImGui::Text("Extracting... saved %d frames", savedCount.load()); 
            ImGui::ProgressBar((float)savedCount.load() / totalFrames.load(), ImVec2(300, 20));
        }
        else if (!videoPath.empty() && extractionDone) {
            ImGui::Text("Extraction complete! Total saved: %d", savedCount.load());
        }

        if (ImGui::Button("Exit")) glfwSetWindowShouldClose(window, true);

        ImGui::End();

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    if (textureID) glDeleteTextures(1, &textureID);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
