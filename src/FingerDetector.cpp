#include "FingerDetector.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

double Distance(const cv::Point& a, const cv::Point& b) {
	return std::hypot(double(a.x - b.x), double(a.y - b.y));
}

// Angle at `far` between the lines to `start` and `end`, in degrees
double AngleDeg(const cv::Point& start, const cv::Point& far, const cv::Point& end) {
	double a = Distance(start, far);
	double b = Distance(end, far);
	double c = Distance(start, end);
	if (a == 0 || b == 0) {
		return 180.0;
	}
	double cosv = std::clamp((a * a + b * b - c * c) / (2 * a * b), -1.0, 1.0);
	return std::acos(cosv) * 180.0 / CV_PI;
}

// Adds p unless a point already exists within minDist
void AddUnique(std::vector<cv::Point>& pts, const cv::Point& p, double minDist) {
	for (const auto& q : pts) {
		if (Distance(p, q) < minDist) {
			return;
		}
	}
	pts.push_back(p);
}

} // namespace

FingerDetector::FingerDetector(cv::Scalar skinLow, cv::Scalar skinHigh, double minHandArea)
	: low(skinLow), high(skinHigh), minArea(minHandArea) {}

cv::Mat FingerDetector::SkinMask(const cv::Mat& frame) const {
	cv::Mat blurred, ycrcb, mask;
	cv::GaussianBlur(frame, blurred, cv::Size(5, 5), 0);
	cv::cvtColor(blurred, ycrcb, cv::COLOR_BGR2YCrCb);
	cv::inRange(ycrcb, low, high, mask);

	// Clean up noise and fill small holes
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
	cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 2);
	cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
	cv::GaussianBlur(mask, mask, cv::Size(5, 5), 0);
	cv::threshold(mask, mask, 127, 255, cv::THRESH_BINARY);
	return mask;
}

HandInfo FingerDetector::Detect(const cv::Mat& frame) const {
	HandInfo hand;
	if (frame.empty()) {
		return hand;
	}
	hand.mask = SkinMask(frame);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(hand.mask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	if (contours.empty()) {
		return hand;
	}

	// Assume the largest skin blob is the hand
	auto largest = std::max_element(contours.begin(), contours.end(),
		[](const auto& a, const auto& b) { return cv::contourArea(a) < cv::contourArea(b); });
	if (cv::contourArea(*largest) < minArea) {
		return hand;
	}
	hand.found = true;
	hand.contour = *largest;

	// Palm center = point inside the hand farthest from its edge
	cv::Mat handMask = cv::Mat::zeros(hand.mask.size(), CV_8UC1);
	cv::drawContours(handMask, std::vector<std::vector<cv::Point>>{hand.contour}, 0, 255, cv::FILLED);
	cv::Mat dist;
	cv::distanceTransform(handMask, dist, cv::DIST_L2, 5);
	double maxDist;
	cv::minMaxLoc(dist, nullptr, &maxDist, nullptr, &hand.palmCenter);
	hand.palmRadius = static_cast<float>(maxDist);

	cv::convexHull(hand.contour, hand.hull);

	std::vector<int> hullIdx;
	cv::convexHull(hand.contour, hullIdx, false, false);
	std::sort(hullIdx.begin(), hullIdx.end()); // convexityDefects needs monotonic indices

	std::vector<cv::Vec4i> defects;
	if (hullIdx.size() > 3 && hand.contour.size() > 3) {
		try {
			cv::convexityDefects(hand.contour, hullIdx, defects);
		} catch (const cv::Exception&) {
			defects.clear(); // self-intersecting contours can throw; just skip this frame
		}
	}

	const double minDepth = hand.palmRadius * 0.6; // ignore shallow wrinkles
	const double tipMergeDist = hand.palmRadius * 0.5;

	for (const auto& d : defects) {
		cv::Point start = hand.contour[d[0]];
		cv::Point end = hand.contour[d[1]];
		cv::Point far = hand.contour[d[2]];
		double depth = d[3] / 256.0;

		// A gap between two fingers is deep and narrow (< 90 degrees)
		if (depth < minDepth || AngleDeg(start, far, end) > 90.0) {
			continue;
		}
		// Valleys below the palm center are usually the wrist/arm, not fingers
		if (far.y > hand.palmCenter.y + hand.palmRadius) {
			continue;
		}
		hand.valleys.push_back(far);
		AddUnique(hand.fingertips, start, tipMergeDist);
		AddUnique(hand.fingertips, end, tipMergeDist);
	}

	// No valleys: either a fist or a single raised finger.
	// A single finger sticks out well beyond the palm radius above the palm.
	if (hand.valleys.empty()) {
		cv::Point farthest = hand.palmCenter;
		double best = 0;
		for (const auto& p : hand.hull) {
			if (p.y >= hand.palmCenter.y) {
				continue; // only look above the palm
			}
			double dd = Distance(p, hand.palmCenter);
			if (dd > best) {
				best = dd;
				farthest = p;
			}
		}
		if (best > hand.palmRadius * 2.0) {
			hand.fingertips.push_back(farthest);
		}
	}

	hand.fingerCount = std::min<int>(static_cast<int>(hand.fingertips.size()), 5);
	return hand;
}

void FingerDetector::Draw(cv::Mat& frame, const HandInfo& hand) {
	if (!hand.found) {
		cv::putText(frame, "No hand", cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
		            cv::Scalar(0, 0, 255), 2);
		return;
	}

	cv::drawContours(frame, std::vector<std::vector<cv::Point>>{hand.contour}, 0,
	                 cv::Scalar(0, 255, 0), 2);
	cv::polylines(frame, hand.hull, true, cv::Scalar(255, 0, 0), 2);

	cv::circle(frame, hand.palmCenter, static_cast<int>(hand.palmRadius), cv::Scalar(255, 255, 0), 2);
	cv::circle(frame, hand.palmCenter, 4, cv::Scalar(255, 255, 0), cv::FILLED);

	for (const auto& v : hand.valleys) {
		cv::circle(frame, v, 6, cv::Scalar(0, 0, 255), cv::FILLED);
	}
	for (size_t i = 0; i < hand.fingertips.size(); ++i) {
		cv::circle(frame, hand.fingertips[i], 10, cv::Scalar(0, 255, 255), cv::FILLED);
		cv::line(frame, hand.palmCenter, hand.fingertips[i], cv::Scalar(0, 255, 255), 1);
		cv::putText(frame, std::to_string(i + 1), hand.fingertips[i] + cv::Point(8, -8),
		            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
	}

	cv::putText(frame, "Fingers: " + std::to_string(hand.fingerCount), cv::Point(10, 30),
	            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
}
