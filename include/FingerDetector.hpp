#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

// Detects a hand via skin color and counts the raised fingers using
// convex hull + convexity defects

struct HandInfo {
	bool found = false;
	int fingerCount = 0;
	std::vector<cv::Point> contour;    // hand outline
	std::vector<cv::Point> hull;       // convex hull of the outline
	std::vector<cv::Point> fingertips; // detected fingertip positions
	std::vector<cv::Point> valleys;    // gaps between fingers
	cv::Point palmCenter;
	float palmRadius = 0.f;
	cv::Mat mask;                      // binary skin mask (for debugging)
};

class FingerDetector {
public:
	// Skin range in YCrCb space; defaults work for most skin tones in decent light
	FingerDetector(cv::Scalar skinLow = cv::Scalar(0, 133, 77),
	               cv::Scalar skinHigh = cv::Scalar(255, 173, 127),
	               double minHandArea = 5000.0);

	HandInfo Detect(const cv::Mat& frame) const;

	// Draws contour, hull, fingertips, palm and finger count onto frame
	static void Draw(cv::Mat& frame, const HandInfo& hand);

private:
	cv::Mat SkinMask(const cv::Mat& frame) const;

	cv::Scalar low;
	cv::Scalar high;
	double minArea;
};
