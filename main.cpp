#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char** argv) {
    // Check if video path is given
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return -1;
    }

    std::string videoPath = argv[1];

    // Open the video file
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video file: " << videoPath << std::endl;
        return -1;
    }

    int frameCount = 0;
    cv::Mat frame;

    while (true) {
        // Read a new frame
        bool success = cap.read(frame);
        if (!success) {
            std::cout << "Finished extracting frames." << std::endl;
            break;
        }


        // Save frame as image
        std::string filename = "frame_" + std::to_string(frameCount) + ".jpg";
        cv::imwrite(filename, frame);

        std::cout << "Saved " << filename << std::endl;
        frameCount++;
    }

    cap.release();
    return 0;
}
