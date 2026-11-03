#include "Detector.hpp"
#include <iostream>

Detector::Detector(const std::unordered_map<std::string, Key>& gTk, double td)
	: threshold(td) {
	for (const auto& [path, key] : gTk) {
		cv::Mat img = cv::imread(path);
		if (img.empty()) {
			std::cerr << "Detector: could not load template '" << path << "'\n";
			continue;
		}
		cv::Mat gray;
		cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
		gestures.push_back({path, gray, key});
	}
}

Key Detector::Detect(const cv::Mat& frame) {
	if (frame.empty() || gestures.empty()) {
		return 0;
	}

	cv::Mat grayFrame;
	cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

	for (const auto& g : gestures) {
		// matchTemplate requires the template to fit inside the frame
		if (g.grayTemplate.cols > grayFrame.cols || g.grayTemplate.rows > grayFrame.rows) {
			continue;
		}
		cv::Mat result;
		cv::matchTemplate(grayFrame, g.grayTemplate, result, cv::TM_CCOEFF_NORMED);
		double minVal, maxVal;
		cv::Point minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
		if (maxVal > threshold) {
			return g.key;
		}
	}
	return 0;
}