#pragma once
#include <opencv2/opencv.hpp>

// Captures webcam live to feed into the detector

class Capture {
public:
	Capture(int frameWidth, int frameHeight);

	// Checks if webcam is connected/detected
	bool ConnectWebcam();

	// Returns the next frame (empty Mat if the read failed)
	cv::Mat GetFrame();

private:
	cv::VideoCapture capture;
	int frameW;
	int frameH;
	bool isConnected;
};