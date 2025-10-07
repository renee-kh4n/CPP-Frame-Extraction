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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <windows.h>
#include <commdlg.h>

namespace fs = std::filesystem;

// ======================
// SafeQueue for frames
// ======================
template <typename T>
class SafeQueue {
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
public:
    void push(T value) {
        {
            std::unique_lock<std::mutex> lock(m);
            q.push(std::move(value));
        }
        cv.notify_one();
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(m);
        if (q.empty()) return false;
        value = std::move(q.front());
        q.pop();
        return true;
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(m);
        return q.empty();
    }
};

// ======================
// Helper: File dialog
// ======================
std::string openFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
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
// OpenCV texture helper
// ======================

    // Define GL_BGR and GL_BGRA for Windows if not included by default
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

    GLenum inputColorFormat = (mat.channels() == 3) ? GL_BGR : GL_BGRA;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mat.cols, mat.rows, 0, inputColorFormat, GL_UNSIGNED_BYTE, mat.ptr());
    return textureID;
}

// ======================
// Main program
// ======================
int main() {
    if (!glfwInit()) return -1;
    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Multithreaded Frame Extractor", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    // State variables
    std::string videoPath;
    std::string outputDir;
    cv::VideoCapture cap;
    SafeQueue<std::pair<int, cv::Mat>> frameQueue;
    std::atomic<bool> extracting = false;
    std::atomic<bool> extractionDone = false;
    std::atomic<int> savedCount = 0;
    std::atomic<int> totalFrames = 0;
    std::atomic<double> fps = 0;
    GLuint textureID = 0;
    cv::Mat displayFrame;

    std::string build_path = fs::current_path().string();

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
                outputDir = fs::path(videoPath).stem().string() + "_frames";
                fs::current_path(build_path);
                fs::create_directory(outputDir);
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
                std::thread([&]() {
                    cv::Mat frame;
                    int frameIndex = 0;
                    while (cap.read(frame)) {
                        frameQueue.push({ frameIndex++, frame.clone() });
                    }
                    extractionDone = true;
                    cap.release();
                    }).detach();

                std::thread([&]() {
                    while (!extractionDone || !frameQueue.empty()) {
                        std::pair<int, cv::Mat> item;
                        if (frameQueue.pop(item)) {
                            std::string filename = outputDir + "/frame_" + std::to_string(item.first) + ".png";
                            cv::imwrite(filename, item.second);
                            savedCount++;
                        }
                        else {
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        }
                    }
                    // Run rembg batch
                    fs::path scriptPath = fs::current_path().parent_path() / "remove_bg.py";
                    std::string command = "python \"" + scriptPath.string() + "\" \"" + outputDir + "\"";
                    system(command.c_str());
                    extracting = false;
                    }).detach();
            }
        }

        if (extracting) {
            ImGui::Text("Extracting... saved %d frames", savedCount.load());
            if (!frameQueue.empty()) {
                auto qsize = savedCount.load();
                ImGui::ProgressBar((float)qsize / totalFrames.load(), ImVec2(300, 20));
            }
        }
        else if (!videoPath.empty() && extractionDone) {
            ImGui::Text("Extraction complete. Total saved: %d", savedCount.load());
            ImGui::Text("Output folder: %s", outputDir.c_str());
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

    if (textureID) glDeleteTextures(1, &textureID);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
