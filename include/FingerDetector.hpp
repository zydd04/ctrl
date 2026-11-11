#pragma once

#include <array>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

// Finds a hand in a camera frame and tells which fingers are raised.
// Uses the two MediaPipe hand models:
//   1. the palm model finds where the hand is
//   2. the landmark model finds 21 points on the hand
//
// The 21 points: 0 = wrist, 1-4 = thumb, 5-8 = index, 9-12 = middle,
// 13-16 = ring, 17-20 = pinky. The last point of each finger is its tip.

struct HandInfo {
	bool found = false;
	int fingerCount = 0;                 // how many fingers are raised
	std::array<bool, 5> fingerUp{};      // thumb, index, middle, ring, pinky
	std::vector<cv::Point2f> landmarks;  // the 21 points, in frame pixels
};

class FingerDetector {
public:
	FingerDetector();

	// False if the model files could not be loaded
	bool IsReady() const;

	HandInfo Detect(const cv::Mat& frame);

	// Draws the hand and the finger count on the frame
	static void Draw(cv::Mat& frame, const HandInfo& hand);

private:
	// Finds 7 points on the palm. Returns false if there is no hand.
	bool FindPalm(const cv::Mat& frame, std::vector<cv::Point2f>& palm);

	// Finds the 21 hand points, starting from the 7 palm points
	bool FindLandmarks(const cv::Mat& frame, const std::vector<cv::Point2f>& palm, HandInfo& hand);

	void CountFingers(HandInfo& hand);

	cv::dnn::Net palmNet;
	cv::dnn::Net handNet;
	std::vector<cv::Point2f> anchors; // grid positions the palm model works with
	bool ready = false;

	// The palm points of the last frame. While we still see the hand we reuse
	// them, so the palm model only runs when the hand is lost.
	bool hasLastPalm = false;
	std::vector<cv::Point2f> lastPalm;
};
