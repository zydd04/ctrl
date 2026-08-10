#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>
#include <chrono>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "Capture.hpp"
#include "Input.hpp"
#include <vector>

//Detects actual hand gestures from the Webcam capture and translates them into key press values

class Detector {
public:
	explicit Detector(std::unordered_map<std::string, WORD> gTk, double td);

	//Detects hand gesture type and returns key
	WORD Detect(cv::Mat& frame);

private:
	std::unordered_map<std::string, WORD> gestureToKey; //Will have for example: Gesture = VK_SPACE
	double treshhold;
	
};


