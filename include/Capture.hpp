#pragma once
#include <opencv2/opencv.hpp>

//Captures webcam live to feed into the detector

class Capture {
public:
	//Constructor
	Capture(int frameWidth, int frameHeight);

	//Checks if Webcam is connected/detected
	bool ConnectWebcam();

	//Captures Frames, returns False if frames are interupted
	cv::Mat GetFrame();

private:
	cv::VideoCapture capture;
	int frameW;
	int frameH;
	bool isConnected;
};
	
	
