#include "FingerDetector.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

const int PALM_INPUT = 192; // palm detector input is 192x192
const int HAND_INPUT = 224; // landmark model input is 224x224
const int NUM_LANDMARKS = 21;

// Hand landmark indices matching the palm detector's 7 keypoints
const int PALM_LANDMARK_IDS[7] = {0, 5, 9, 13, 17, 1, 2};

// How the upright hand crop is built around the palm points
const float CROP_SHIFT_UP = 0.4f; // move the crop towards the fingers
const float CROP_ENLARGE = 3.0f;  // palm box -> whole hand

const int SKELETON[][2] = {
	{0, 1}, {1, 2}, {2, 3}, {3, 4},         // thumb
	{0, 5}, {5, 6}, {6, 7}, {7, 8},         // index
	{9, 10}, {10, 11}, {11, 12},            // middle
	{13, 14}, {14, 15}, {15, 16},           // ring
	{0, 17}, {17, 18}, {18, 19}, {19, 20},  // pinky
	{5, 9}, {9, 13}, {13, 17},              // palm
};

float Distance(const cv::Point2f& a, const cv::Point2f& b) {
	return std::hypot(a.x - b.x, a.y - b.y);
}

// Both models take RGB float images in NHWC layout, values 0..1
cv::Mat ToBlob(const cv::Mat& bgr) {
	cv::Mat rgb, f;
	cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
	rgb.convertTo(f, CV_32F, 1.0 / 255.0);
	int dims[4] = {1, f.rows, f.cols, 3};
	return cv::Mat(4, dims, CV_32F, f.data).clone();
}

// SSD anchors of the MediaPipe palm detector: a 24x24 grid with 2 anchors per
// cell followed by a 12x12 grid with 6 anchors per cell (2016 in total)
std::vector<cv::Point2f> MakeAnchors() {
	std::vector<cv::Point2f> anchors;
	const int grids[2] = {24, 12};
	const int perCell[2] = {2, 6};
	for (int g = 0; g < 2; ++g) {
		for (int y = 0; y < grids[g]; ++y) {
			for (int x = 0; x < grids[g]; ++x) {
				for (int n = 0; n < perCell[g]; ++n) {
					anchors.emplace_back((x + 0.5f) / grids[g], (y + 0.5f) / grids[g]);
				}
			}
		}
	}
	return anchors;
}

} // namespace

FingerDetector::FingerDetector(const std::string& modelDir, float palmScoreThreshold,
                               float handScoreThreshold)
	: palmThreshold(palmScoreThreshold), handThreshold(handScoreThreshold) {
	try {
		palmNet = cv::dnn::readNet(modelDir + "/palm_detection_mediapipe_2023feb.onnx");
		handNet = cv::dnn::readNet(modelDir + "/handpose_estimation_mediapipe_2023feb.onnx");
		palmOutputs = palmNet.getUnconnectedOutLayersNames();
		handOutputs = handNet.getUnconnectedOutLayersNames();
		anchors = MakeAnchors();
		ready = true;
	} catch (const cv::Exception& e) {
		std::cerr << "FingerDetector: could not load the MediaPipe hand models from '"
		          << modelDir << "' (re-run cmake to download them)\n" << e.what() << "\n";
	}
}

bool FingerDetector::DetectPalm(const cv::Mat& frame, PalmPoints& palm) {
	// Letterbox the frame into the square network input
	float ratio = std::min(float(PALM_INPUT) / frame.cols, float(PALM_INPUT) / frame.rows);
	cv::Size scaled(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio));
	int padX = (PALM_INPUT - scaled.width) / 2;
	int padY = (PALM_INPUT - scaled.height) / 2;
	cv::Mat input = cv::Mat::zeros(PALM_INPUT, PALM_INPUT, frame.type());
	cv::resize(frame, input(cv::Rect(padX, padY, scaled.width, scaled.height)), scaled);

	palmNet.setInput(ToBlob(input));
	std::vector<cv::Mat> outs;
	palmNet.forward(outs, palmOutputs);

	// One output holds box + keypoint offsets (N x 18), the other raw scores (N x 1)
	const cv::Mat* deltas = nullptr;
	const cv::Mat* scores = nullptr;
	for (const auto& o : outs) {
		(o.size[o.dims - 1] == 18 ? deltas : scores) = &o;
	}
	if (!deltas || !scores || scores->total() != anchors.size()) {
		return false;
	}

	// Only one hand is needed, so just take the highest scoring anchor
	const float* s = scores->ptr<float>();
	int best = static_cast<int>(std::max_element(s, s + anchors.size()) - s);
	float score = 1.f / (1.f + std::exp(-s[best]));
	if (score < palmThreshold) {
		return false;
	}

	// Keypoints are offsets from the anchor in input pixels; map back to the frame
	const float* d = deltas->ptr<float>() + best * 18;
	for (int i = 0; i < 7; ++i) {
		float x = d[4 + 2 * i] + anchors[best].x * PALM_INPUT;
		float y = d[5 + 2 * i] + anchors[best].y * PALM_INPUT;
		palm[i] = cv::Point2f((x - padX) / ratio, (y - padY) / ratio);
	}
	return true;
}

bool FingerDetector::EstimateLandmarks(const cv::Mat& frame, const PalmPoints& palm,
                                       HandInfo& hand) {
	// Hand axes in the frame: `down` points from the middle finger base to the
	// wrist, `right` is perpendicular to it. Cropping along them makes the hand upright.
	cv::Point2f up = palm[2] - palm[0];
	float len = std::hypot(up.x, up.y);
	if (len < 1.f) {
		return false;
	}
	up *= 1.f / len;
	cv::Point2f down = -up;
	cv::Point2f right(-up.y, up.x);

	// Bounding box of the palm points in hand axes
	float minA = 1e9f, maxA = -1e9f, minB = 1e9f, maxB = -1e9f;
	for (const auto& p : palm) {
		float a = p.dot(right), b = p.dot(down);
		minA = std::min(minA, a); maxA = std::max(maxA, a);
		minB = std::min(minB, b); maxB = std::max(maxB, b);
	}
	float w = maxA - minA, h = maxB - minB;
	float side = CROP_ENLARGE * std::max(w, h);
	if (side < 20.f) {
		return false;
	}
	float ca = (minA + maxA) / 2;
	float cb = (minB + maxB) / 2 - CROP_SHIFT_UP * h;
	cv::Point2f center = ca * right + cb * down;

	// Affine transform from crop pixels to frame pixels
	float s = side / HAND_INPUT;
	float half = HAND_INPUT / 2.f;
	cv::Matx23f cropToFrame(
		right.x * s, down.x * s, center.x - half * s * (right.x + down.x),
		right.y * s, down.y * s, center.y - half * s * (right.y + down.y));

	cv::Mat crop;
	cv::warpAffine(frame, crop, cropToFrame, cv::Size(HAND_INPUT, HAND_INPUT),
	               cv::INTER_LINEAR | cv::WARP_INVERSE_MAP, cv::BORDER_CONSTANT);

	handNet.setInput(ToBlob(crop));
	std::vector<cv::Mat> outs;
	handNet.forward(outs, handOutputs);
	// Outputs: landmarks (63), hand presence (1), handedness (1), world landmarks (63)
	if (outs.size() < 2 || outs[0].total() != NUM_LANDMARKS * 3 || outs[1].total() != 1) {
		return false;
	}

	hand.confidence = outs[1].ptr<float>()[0];
	if (hand.confidence < handThreshold) {
		return false;
	}

	const float* lm = outs[0].ptr<float>(); // x, y, z per landmark, in crop pixels
	hand.landmarks.resize(NUM_LANDMARKS);
	for (int i = 0; i < NUM_LANDMARKS; ++i) {
		float x = lm[3 * i], y = lm[3 * i + 1];
		hand.landmarks[i] = cv::Point2f(
			cropToFrame(0, 0) * x + cropToFrame(0, 1) * y + cropToFrame(0, 2),
			cropToFrame(1, 0) * x + cropToFrame(1, 1) * y + cropToFrame(1, 2));
	}
	hand.box = cv::boundingRect(hand.landmarks);

	// A tracked crop can drift off the hand onto nothing; drop it if it left the frame
	if ((hand.box & cv::Rect(0, 0, frame.cols, frame.rows)).area() < hand.box.area() / 2) {
		return false;
	}
	hand.found = true;
	return true;
}

void FingerDetector::CountFingers(HandInfo& hand) {
	const auto& p = hand.landmarks;
	const cv::Point2f& wrist = p[0];

	// Thumb: when raised the tip moves away from the palm, when folded it tucks
	// in towards the pinky/index bases
	hand.fingerUp[0] = Distance(p[4], p[17]) > Distance(p[3], p[17]) &&
	                   Distance(p[4], p[5]) > Distance(p[3], p[5]);

	// Other fingers: a raised finger has its tip farther from the wrist than its
	// middle joint; a curled one brings the tip back. Works at any hand rotation.
	for (int f = 1; f < 5; ++f) {
		int mcp = 4 * f + 1, pip = mcp + 1, tip = mcp + 3;
		float tipDist = Distance(p[tip], wrist);
		hand.fingerUp[f] = tipDist > Distance(p[pip], wrist) &&
		                   tipDist > Distance(p[mcp], wrist);
	}

	for (int f = 0; f < 5; ++f) {
		if (hand.fingerUp[f]) {
			++hand.fingerCount;
			hand.fingertips.emplace_back(p[4 * f + 4]);
		}
	}
}

HandInfo FingerDetector::Detect(const cv::Mat& frame) {
	HandInfo hand;
	if (!ready || frame.empty()) {
		return hand;
	}

	// Follow the hand from the previous frame; fall back to the palm detector
	// when there was none or the landmark model no longer sees a hand there
	bool ok = tracking && EstimateLandmarks(frame, lastPalm, hand);
	if (!ok) {
		hand = HandInfo();
		PalmPoints palm;
		ok = DetectPalm(frame, palm) && EstimateLandmarks(frame, palm, hand);
	}
	if (!ok) {
		tracking = false;
		return HandInfo();
	}

	for (int i = 0; i < 7; ++i) {
		lastPalm[i] = hand.landmarks[PALM_LANDMARK_IDS[i]];
	}
	tracking = true;

	CountFingers(hand);
	return hand;
}

void FingerDetector::Draw(cv::Mat& frame, const HandInfo& hand) {
	if (!hand.found) {
		cv::putText(frame, "No hand", cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
		            cv::Scalar(0, 0, 255), 2);
		return;
	}

	for (const auto& bone : SKELETON) {
		cv::line(frame, hand.landmarks[bone[0]], hand.landmarks[bone[1]],
		         cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
	}
	for (const auto& p : hand.landmarks) {
		cv::circle(frame, p, 4, cv::Scalar(255, 0, 0), cv::FILLED, cv::LINE_AA);
	}
	for (size_t i = 0; i < hand.fingertips.size(); ++i) {
		cv::circle(frame, hand.fingertips[i], 9, cv::Scalar(0, 255, 255), cv::FILLED, cv::LINE_AA);
		cv::putText(frame, std::to_string(i + 1), hand.fingertips[i] + cv::Point(10, -10),
		            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
	}

	cv::putText(frame, "Fingers: " + std::to_string(hand.fingerCount), cv::Point(10, 30),
	            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
}
