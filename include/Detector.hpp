#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>
#include <chrono>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "Capture.hpp"
#include "Input.hpp"
#include <thread>

//Detects actual hand gestures from the Webcam capture and translates them into key press values

class Detector {
public:
	explicit Detector(std::string path, double td);

	//Detects hand gesture type and returns key
	WORD Detect(cv::Mat& frame);

private:
	std::string PathImg;
	double treshhold;
};


