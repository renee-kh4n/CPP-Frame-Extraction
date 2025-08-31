#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
	std::string videoPath;

	std::cout << "Please enter the path to your video: ";
	std::getline(std::cin, videoPath);

	cv::VideoCapture videoCap(videoPath);

	if (!videoCap.isOpened()) {
		std::cerr << "Error: Cannot open video file from " << videoPath << ". Please check the path name or if the video exists." << std::endl;
		return -1;
	}

	double fps = videoCap.get(cv::CAP_PROP_FPS); // gets the frame rate
	double totalFrames = videoCap.get(cv::CAP_PROP_FRAME_COUNT);
	double duration = totalFrames / fps;

	std::cout << "Video Duration: " << duration << " seconds" << std::endl;
	std::cout << "Frame Rate (FPS): " << fps << std::endl;
	std::cout << "Total Number of Frames: " << totalFrames << std::endl;

	cv::Mat frame; // cv::Mat = matrix that stores image data for image manipulation

	int frameCount = 0;
	int savedCount = 0;
	std::string filename;
	bool success = videoCap.read(frame);

	while (success) {

		if (frameCount % static_cast<int>(fps) == 0) { // (int)fps is the C-style cast (older, less explicit).
			filename = "frame_" + std::to_string(savedCount) + ".png";
			cv::imwrite(filename, frame); // saves an image to a specified filename

			std::cout << "Saved " << filename << std::endl;

			savedCount++;
		}

		frameCount++;

		success = videoCap.read(frame);
	}

	videoCap.release();
	//	Closes the video file or camera.
	//	Frees up memory and system handles.
	//	Makes the resource available for other applications.



	std::cout << "Finished saving " << savedCount << " frames. ";

	return 0;

}
