#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <opencv2/opencv.hpp>
#include "Input.hpp"

// Detects hand gestures in webcam frames and translates them into keys

class Detector {
public:
	// gTk: template image path -> key to press (e.g. {"img.png", XK_space})
	// td:  match threshold (0..1) for TM_CCOEFF_NORMED
	Detector(const std::unordered_map<std::string, Key>& gTk, double td);

	// Returns the key of the first matching gesture, or 0 if none
	Key Detect(const cv::Mat& frame);

private:
	struct Gesture {
		std::string path;
		cv::Mat grayTemplate;
		Key key;
	};

	std::vector<Gesture> gestures; // templates loaded once, not every frame
	double threshold;
};