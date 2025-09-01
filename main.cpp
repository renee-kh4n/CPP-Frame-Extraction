#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>

#include <windows.h>
#include <commdlg.h>

namespace fs = std::filesystem;


std::string openFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Video Files\0*.mp4;*.avi;*.mov\0All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn)) {
        return std::string(filename);
    }
    return "";
}


// Convert cv::Mat (BGR) to OpenGL texture for ImGui
GLuint matToTexture(const cv::Mat& mat) {
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rgb.cols, rgb.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);

    return textureID;
}

int main() {
    // Setup GLFW
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Video Frame Extractor", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui::StyleColorsDark();

    // App variables
    char videoPath[256] = "";

    bool extracting = false;
    bool finished = false;

    int savedCount = 0;
    int frameCount = 0;
    double fps = 0;
    double totalFrames = 0;
    double duration = 0;

    cv::VideoCapture cap;
    cv::Mat frame;
    GLuint textureID = 0;

    std::string folderName = "";

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Get the main window size from GLFW
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Position & size for side-mounted panel
        ImGui::SetNextWindowPos(ImVec2(0, 0));  // stick to left/top
        ImGui::SetNextWindowSize(ImVec2( (float)display_w, (float)display_h )); // full width width, full height

        // Disable resizing & moving
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        ImGui::Begin("Video Frame Extractor", nullptr, window_flags);

    
        if (ImGui::Button("Choose Video File")) {
            std::string chosen = openFileDialog();
            if (!chosen.empty()) {
                strncpy(videoPath, chosen.c_str(), sizeof(videoPath));
            }
            std::cout << "Video File: " << videoPath << std::endl;

            cap.open(videoPath);
            if (cap.isOpened()) {
                fps = cap.get(cv::CAP_PROP_FPS);
                totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
                savedCount = 0;
                frameCount = 0;

                fs::path videoPathObj(videoPath);
                folderName = videoPathObj.stem().string();
                fs::create_directory(folderName);

                extracting = true;
            }
            else {
                std::cerr << "Error: Cannot open video file.\n";
                ImGui::Text("Please choose a video file.");
            }

        }

        if (fps > 0 && totalFrames > 0) {
            duration = totalFrames / fps;
        }

        ImGui::Text("Video Path: %s", videoPath);
        ImGui::Text("Video Duration: %.0f seconds", duration);
        ImGui::Text("Frame Rate: %.0f", fps);
        ImGui::Text("Total Number of frames: %.0f", totalFrames);
        ImGui::Text("");

        if (ImGui::Button("Start Extraction")) {
            extracting = true;
           
        }

        if (extracting) {
            if (cap.read(frame)) {
                frameCount++;

                // Save 1 frame per second
                if (frameCount % static_cast<int>(fps) == 0) {
                    std::string filename = "frame_" + std::to_string(savedCount) + ".png";

                    cv::imwrite(fs::current_path().string() + "/" + folderName + "/" + filename, frame);
                    savedCount++;
                    std::cout << " File: " << filename << std::endl;
                }

                if (textureID) glDeleteTextures(1, &textureID);
                textureID = matToTexture(frame);

                ImGui::Text("Extracting... Saved %d frames", savedCount);
                ImGui::Text("Reading Frame %d / %.0f", frameCount, totalFrames);
                ImGui::Text("FPS: %.2f", fps);

                if (textureID) {
                    ImGui::Image((void*)(intptr_t)textureID, ImVec2(640, 360));
                }

            }
            else {
                extracting = false;
                finished = true;
                cap.release();
            }
        }

        fs::path videoPathObj(videoPath);
        std::string folderName = videoPathObj.stem().string();
        //fs::create_directory(folderName);


        if (finished) {
            ImGui::Text("Finished saving %d frames.", savedCount);

            ImGui::Text("The frames are saved in: ", fs::absolute(folderName).string().c_str());

        }

        if (ImGui::Button("Done")) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
       

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w2, display_h2;
        glfwGetFramebufferSize(window, &display_w2, &display_h2);
        glViewport(0, 0, display_w2, display_h2);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    if (textureID) glDeleteTextures(1, &textureID);
    cap.release();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
