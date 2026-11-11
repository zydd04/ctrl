#pragma once

#include <array>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

// Finds the hand with the 2 MediaPipe models (palm, then 21 points)
// Points: 0 = wrist, 1-4 = thumb, 5-8 = index, 9-12 = middle, 13-16 = ring, 17-20 = pinky

struct HandInfo {
	bool found = false;
	int fingerCount = 0;
	std::array<bool, 5> fingerUp{}; // thumb, index, middle, ring, pinky
	std::vector<cv::Point2f> landmarks; // 21 points, in frame pixels
};

class FingerDetector {
public:
	FingerDetector();

	bool IsReady() const;

	HandInfo Detect(const cv::Mat& frame);

	static void Draw(cv::Mat& frame, const HandInfo& hand);

private:
	bool FindPalm(const cv::Mat& frame, std::vector<cv::Point2f>& palm);

	bool FindLandmarks(const cv::Mat& frame, const std::vector<cv::Point2f>& palm, HandInfo& hand);

	void CountFingers(HandInfo& hand);

	cv::dnn::Net palmNet;
	cv::dnn::Net handNet;
	std::vector<cv::Point2f> anchors; // grid points of the palm model
	bool ready = false;

	// Palm of the last frame, reused while the hand is still seen
	std::vector<cv::Point2f> lastPalm;
};
