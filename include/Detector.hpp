#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>
#include <chrono>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "Capture.hpp"
#include "InputController.hpp"

//Detects actual hand gestures from the Webcam capture and translates them into key press values

class Detector {
public:
	explicit Detector(std::unordered_map<std::vector<std:string>,WORD> gestureToKey, int threshhold);

	//Detects hand gesture type and returns key
	WORD Detect(cv::Mat& frame) const {return keyPress;}

	//Outputs Fps counter for the Webcam capture
	void DrawFps(cv::Mat& frame); 

private:
	std::unordred_map<std::vector<std::string>, WORD> gestureToKey;
	WORD keyPress;
	int threshhold;

};


