#include "FingerDetector.hpp"

#include <algorithm>
#include <cmath>
#include <string>

const int PALM_SIZE = 192; // input size of the palm model
const int HAND_SIZE = 224; // input size of the landmark model

const float PALM_MIN_SCORE = 0.5f;
const float HAND_MIN_SCORE = 0.7f;

static float Distance(cv::Point2f a, cv::Point2f b) {
	return cv::norm(a - b);
}

// Models want RGB from 0 to 1
static cv::Mat MakeInput(const cv::Mat& image) {
	cv::Mat rgb;
	cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
	rgb.convertTo(rgb, CV_32F, 1.0 / 255.0);

	int shape[4] = {1, rgb.rows, rgb.cols, 3};
	return cv::Mat(4, shape, CV_32F, rgb.data).clone();
}

FingerDetector::FingerDetector() {
	std::string folder = CTRL_MODEL_DIR;
	try {
		palmNet = cv::dnn::readNet(folder + "/palm_detection_mediapipe_2023feb.onnx");
		handNet = cv::dnn::readNet(folder + "/handpose_estimation_mediapipe_2023feb.onnx");
		ready = true;
	} catch (const cv::Exception&) {
		// ready stays false
	}

	// Grid points of the palm model
	const int grids[2][2] = {{24, 2}, {12, 6}}; // grid size, points per cell
	for (auto& [size, perCell] : grids) {
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				for (int i = 0; i < perCell; i++) {
					anchors.push_back(cv::Point2f((x + 0.5f) / size, (y + 0.5f) / size));
				}
			}
		}
	}
}

bool FingerDetector::IsReady() const {
	return ready;
}

bool FingerDetector::FindPalm(const cv::Mat& frame, std::vector<cv::Point2f>& palm) {
	// Fit the frame in a square with black bars
	float scale = std::min((float)PALM_SIZE / frame.cols, (float)PALM_SIZE / frame.rows);
	int width = cvRound(frame.cols * scale);
	int height = cvRound(frame.rows * scale);
	int left = (PALM_SIZE - width) / 2;
	int top = (PALM_SIZE - height) / 2;

	cv::Mat square = cv::Mat::zeros(PALM_SIZE, PALM_SIZE, frame.type());
	cv::Mat small;
	cv::resize(frame, small, cv::Size(width, height));
	small.copyTo(square(cv::Rect(left, top, width, height)));

	std::vector<cv::Mat> outputs;
	palmNet.setInput(MakeInput(square));
	palmNet.forward(outputs, palmNet.getUnconnectedOutLayersNames());

	// 2 outputs: 18 numbers per grid point (box + 7 palm points) and 1 score
	cv::Mat points, scores;
	for (const cv::Mat& output : outputs) {
		if (output.size[output.dims - 1] == 18) {
			points = output;
		} else {
			scores = output;
		}
	}

	// Best score
	const float* score = scores.ptr<float>();
	int best = 0;
	for (int i = 1; i < (int)anchors.size(); i++) {
		if (score[i] > score[best]) {
			best = i;
		}
	}
	float bestScore = 1 / (1 + std::exp(-score[best])); // to 0..1
	if (bestScore < PALM_MIN_SCORE) {
		return false;
	}

	// Its 7 palm points, back in frame pixels
	const float* p = points.ptr<float>() + best * 18;
	palm.clear();
	for (int i = 0; i < 7; i++) {
		float x = p[4 + i * 2] + anchors[best].x * PALM_SIZE;
		float y = p[5 + i * 2] + anchors[best].y * PALM_SIZE;
		palm.push_back(cv::Point2f((x - left) / scale, (y - top) / scale));
	}
	return true;
}

bool FingerDetector::FindLandmarks(const cv::Mat& frame, const std::vector<cv::Point2f>& palm,
                                   HandInfo& hand) {
	// Hand direction: palm[0] = wrist, palm[2] = middle finger base
	cv::Point2f up = palm[2] - palm[0];
	float length = std::hypot(up.x, up.y);
	if (length < 1) {
		return false;
	}
	up = up / length;
	cv::Point2f down = -up;
	cv::Point2f right(-up.y, up.x);

	// Palm size along those directions
	float minRight = 1e9f, maxRight = -1e9f;
	float minDown = 1e9f, maxDown = -1e9f;
	for (int i = 0; i < 7; i++) {
		float r = palm[i].dot(right);
		float d = palm[i].dot(down);
		minRight = std::min(minRight, r);
		maxRight = std::max(maxRight, r);
		minDown = std::min(minDown, d);
		maxDown = std::max(maxDown, d);
	}
	float palmWidth = maxRight - minRight;
	float palmHeight = maxDown - minDown;

	// Square of 3 times the palm, moved towards the fingers
	float side = 3 * std::max(palmWidth, palmHeight);
	if (side < 20) {
		return false;
	}
	float centerRight = (minRight + maxRight) / 2;
	float centerDown = (minDown + maxDown) / 2 - 0.4f * palmHeight;
	cv::Point2f center = centerRight * right + centerDown * down;

	// Square pixel to frame pixel
	float s = side / HAND_SIZE;
	float half = HAND_SIZE / 2.0f;
	cv::Mat toFrame = (cv::Mat_<float>(2, 3) <<
		right.x * s, down.x * s, center.x - half * s * (right.x + down.x),
		right.y * s, down.y * s, center.y - half * s * (right.y + down.y));

	// Cut the square out of the frame
	cv::Mat square;
	cv::warpAffine(frame, square, toFrame, cv::Size(HAND_SIZE, HAND_SIZE),
	               cv::INTER_LINEAR | cv::WARP_INVERSE_MAP);

	// Output 0 = 21 points (x, y, z), output 1 = hand score
	std::vector<cv::Mat> outputs;
	handNet.setInput(MakeInput(square));
	handNet.forward(outputs, handNet.getUnconnectedOutLayersNames());
	if (outputs[1].ptr<float>()[0] < HAND_MIN_SCORE) {
		return false;
	}

	// Points back to frame pixels
	const float* p = outputs[0].ptr<float>();
	std::vector<cv::Point2f> points;
	for (int i = 0; i < 21; i++) {
		points.push_back(cv::Point2f(p[i * 3], p[i * 3 + 1]));
	}
	cv::transform(points, hand.landmarks, toFrame);
	hand.found = true;
	return true;
}

void FingerDetector::CountFingers(HandInfo& hand) {
	const std::vector<cv::Point2f>& p = hand.landmarks;
	cv::Point2f wrist = p[0];

	// Thumb up = tip farther from index base and pinky base than its middle joint
	hand.fingerUp[0] = Distance(p[4], p[17]) > Distance(p[3], p[17]) &&
	                   Distance(p[4], p[5]) > Distance(p[3], p[5]);

	// Other fingers up = tip farther from the wrist than the lower joints
	for (int finger = 1; finger < 5; finger++) {
		int base = finger * 4 + 1;
		int middle = base + 1;
		int tip = base + 3;
		hand.fingerUp[finger] = Distance(p[tip], wrist) > Distance(p[middle], wrist) &&
		                        Distance(p[tip], wrist) > Distance(p[base], wrist);
	}

	hand.fingerCount = std::count(hand.fingerUp.begin(), hand.fingerUp.end(), true);
}

HandInfo FingerDetector::Detect(const cv::Mat& frame) {
	HandInfo hand;

	// Try where the hand was last frame, else search the whole frame
	bool found = !lastPalm.empty() && FindLandmarks(frame, lastPalm, hand);
	if (!found) {
		std::vector<cv::Point2f> palm;
		found = FindPalm(frame, palm) && FindLandmarks(frame, palm, hand);
	}

	lastPalm.clear();
	if (!found) {
		return hand;
	}

	// Remember the palm for the next frame (same 7 points as the palm model)
	for (int i : {0, 5, 9, 13, 17, 1, 2}) {
		lastPalm.push_back(hand.landmarks[i]);
	}

	CountFingers(hand);
	return hand;
}

void FingerDetector::Draw(cv::Mat& frame, const HandInfo& hand) {
	if (!hand.found) {
		cv::putText(frame, "No hand", cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
		            cv::Scalar(0, 0, 255), 2);
		return;
	}

	// Lines of the hand
	const int lines[21][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 4},         // thumb
		{0, 5}, {5, 6}, {6, 7}, {7, 8},         // index
		{9, 10}, {10, 11}, {11, 12},            // middle
		{13, 14}, {14, 15}, {15, 16},           // ring
		{0, 17}, {17, 18}, {18, 19}, {19, 20},  // pinky
		{5, 9}, {9, 13}, {13, 17},              // palm
	};
	for (int i = 0; i < 21; i++) {
		cv::line(frame, hand.landmarks[lines[i][0]], hand.landmarks[lines[i][1]],
		         cv::Scalar(0, 255, 0), 2);
	}

	for (int i = 0; i < 21; i++) {
		cv::circle(frame, hand.landmarks[i], 4, cv::Scalar(255, 0, 0), cv::FILLED);
	}

	// Yellow dot on raised fingertips
	for (int finger = 0; finger < 5; finger++) {
		if (hand.fingerUp[finger]) {
			cv::circle(frame, hand.landmarks[finger * 4 + 4], 9, cv::Scalar(0, 255, 255), cv::FILLED);
		}
	}

	cv::putText(frame, "Fingers: " + std::to_string(hand.fingerCount), cv::Point(10, 30),
	            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
}
