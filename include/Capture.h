#pragma once
#include <opencv2/opencv.hpp>

//Captures webcam live to feed into the detector

class Capture {
public:
	explicit Capture(int frameWidth = 200, int frameHeight = 200);

	//Checks if Webcam is connected/detected
	bool ConnectWebcam(int device_i = 0);

	bool IsConnected() const { return isConnected_; }

	//Captures Frames, returns False if frames are interupted
	bool GetFrame(cv::Mat& outFrame);

private:
	int frameWidth_;
	int frameHeight_;
	bool isConnected_ = false;
};
	
	
