#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>

#include <iostream>
#include <cstdio>
#include <ctime>
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
    if (mat.empty()) return 0;

    cv::Mat input = mat;

    if (mat.channels() == 1) {
        cv::cvtColor(mat, input, cv::COLOR_GRAY2BGR); //why is there an error
    }
    else {
        cv::cvtColor(mat, input, cv::COLOR_BGR2RGB);

    }


    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, input.cols, input.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, input.data);



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

    int savedFPS = 1;

    cv::VideoCapture cap;
    cv::Mat frame, fgMask;
    GLuint textureID = 0, maskTextureID = 0;
    int vidWidth = 0, vidHeight = 0;

    std::string build_path = fs::current_path().string();
    std::string folderName = "";
    //bool directoryCreated = false;
    //fs::path outputDir;

    std::clock_t start = 0, end = 0;
    double processDuration = 0;

    cv::Ptr<cv::BackgroundSubtractor> pBackSub;
    pBackSub = cv::createBackgroundSubtractorKNN();



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
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h)); // full width width, full height

        // Disable resizing & moving
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        ImGui::Begin("Video Frame Extractor", nullptr, window_flags);


        if (ImGui::Button("Choose Video File")) {
            std::string chosen = openFileDialog();
            if (!chosen.empty()) {
                std::strncpy(videoPath, chosen.c_str(), sizeof(videoPath));
            }
            std::cout << "Video File: " << videoPath << std::endl;

            fs::current_path(build_path);  // after dialog
            fs::path videoPathObj(videoPath);
            folderName = videoPathObj.stem().string();
            fs::create_directory(folderName);

            cap.open(videoPath);
            if (cap.isOpened()) {
                fps = cap.get(cv::CAP_PROP_FPS);
                totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
                savedCount = 0;
                frameCount = 0;

                // ✅ Get the width & height of the video here
                vidWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
                vidHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

                /*  extracting = true;*/

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
        ImGui::Text("Frame Rate: %.0f fps", fps);
        ImGui::Text("Total Number of frames: %.0f", totalFrames);
        ImGui::Text("");


        //radio button sample
        ImGui::Text("Frames per Second:");
        ImGui::SameLine();
        for (int i = 1; i <= 5; i++) {
            char label[2];
            snprintf(label, sizeof(label), "%d", i);
            if (ImGui::RadioButton(label, savedFPS == i)) {
                savedFPS = i;
            }
            ImGui::SameLine();
        }
        ImGui::NewLine(); // move to next row after last SameLine()


        ImGui::Text("Saving %d frames per second", savedFPS);

        if (ImGui::Button("Start Extraction")) {
            extracting = true;
            start = std::clock();

        }



        if (extracting) {
            
            if (extracting) {
                if (cap.read(frame)) {
                    frameCount++;

                    // Only process frames we want to save
                    if (frameCount % static_cast<int>(fps / savedFPS) == 0) {
                        // 1. Define a rectangle 10px away from each edge
                      /*  int vidWidth = frame.cols;
                        int vidHeight = frame.rows;*/
                        //cv::Rect rect(50, 50, W - 20, H - 20);  // x, y, width, height

                        int rectWidth = vidWidth - (vidWidth /4);
                        int rectHeight = vidHeight - (vidHeight /4);

                        int x = (vidWidth - rectWidth) / 2;  // center horizontally
                        int y = (vidHeight - rectHeight) / 2; // center vertically

                        cv::Rect rect(x, y, rectWidth, rectHeight);


                        // 2. Initialize mask and models
                        cv::Mat grabMask(frame.size(), CV_8UC1, cv::GC_BGD); // start with all background
                        cv::Mat bgModel, fgModel;

                        // 3. Run GrabCut
                        cv::grabCut(frame, grabMask, rect, bgModel, fgModel, 5, cv::GC_INIT_WITH_RECT);

                        // 4. Create binary alpha mask
                        cv::Mat alpha;
                        alpha = (grabMask == cv::GC_FGD) | (grabMask == cv::GC_PR_FGD);
                        alpha.convertTo(alpha, CV_8UC1, 255);

                        // 5. Merge BGR + alpha
                        std::vector<cv::Mat> channels;
                        cv::split(frame, channels); // B, G, R
                        channels.push_back(alpha);
                        cv::Mat frameTransparent;
                        cv::merge(channels, frameTransparent);

                        // 6. Save PNG
                        std::string filename = "frame_" + std::to_string(savedCount) + ".png";
                        if (!cv::imwrite(folderName + "/" + filename, frameTransparent)) {
                            std::cerr << "Failed to save: " << folderName + "/" << filename << std::endl;
                        }

                        savedCount++;
                        std::cout << "Saved: " << filename << std::endl;

                        //// 7. For display in ImGui
                        //textureID = matToTexture(frame);
                        //maskTextureID = matToTexture(alpha);
                    }

                    textureID = matToTexture(frame);

                    ImGui::Text("Extracting... Saved %d frames", savedCount);
                    ImGui::Text("Reading Frame %d / %.0f", frameCount, totalFrames);
                    if (textureID && maskTextureID) {
                        ImGui::Image((void*)(intptr_t)textureID, ImVec2(640, 360));
                        ImGui::Image((void*)(intptr_t)maskTextureID, ImVec2(640, 360));
                    }
                }
                else {
                    extracting = false;
                    finished = true;
                    cap.release();
                }
            }

        }

        if (finished) {
            ImGui::Text("Finished saving %d frames.", savedCount);

            ImGui::Text("The frames are saved in: %s", fs::absolute(folderName).string().c_str());

            end = std::clock();

            processDuration = (end - start) / (double)CLOCKS_PER_SEC;

            ImGui::Text("Operation took %lf seconds", processDuration);

            std::cout << "Operation took " << processDuration << " seconds" << std::endl;

            finished = false;

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

    if (maskTextureID) glDeleteTextures(1, &maskTextureID);
    cap.release();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
