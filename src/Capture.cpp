#include "Capture.hpp"

Capture::Capture(int frameWidth, int frameHeight)
	: frameW(frameWidth), frameH(frameHeight), isConnected(false) {
	capture.open(0, cv::CAP_V4L2); // Linux webcam backend (/dev/video0)
	if (capture.isOpened()) {
		capture.set(cv::CAP_PROP_FRAME_WIDTH, frameW);
		capture.set(cv::CAP_PROP_FRAME_HEIGHT, frameH);
	}
}

bool Capture::ConnectWebcam() {
	isConnected = capture.isOpened();
	return isConnected;
}

cv::Mat Capture::GetFrame() {
	cv::Mat frame;
	if (capture.isOpened()) {
		capture >> frame;
	}
	return frame;
}