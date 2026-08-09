#include "Capture.hpp"

Capture::Capture(int frameWidth, int frameHeight) {
	frameW = frameWidth;
	frameH = frameHeight;
	capture.open(0);
	isConnected = false;
};

bool Capture::ConnectWebcam() {
	if (capture.isOpened()) {
		isConnected = true;
	}
	return isConnected;
}

cv::Mat Capture::GetFrame() {
	cv::Mat frame;
	capture.set(cv::CAP_PROP_FRAME_WIDTH, frameW);
	capture.set(cv::CAP_PROP_FRAME_HEIGHT, frameH);
	capture >> frame;
	return frame;
}
	

