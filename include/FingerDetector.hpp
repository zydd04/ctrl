#pragma once

#include <array>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

// Detects a hand and counts the raised fingers with MediaPipe's hand models
// (palm detection + 21-point hand landmarks), run through OpenCV DNN.
//
// Pipeline, same as MediaPipe Hands:
//   1. the palm detector finds the hand and its orientation in the full frame
//   2. the hand is cropped upright and the landmark model predicts 21 keypoints
//   3. while the hand stays visible, the next crop is derived from the previous
//      landmarks, so the palm detector only runs when the hand is lost
//
// Landmark indices: 0 wrist, 1-4 thumb, 5-8 index, 9-12 middle, 13-16 ring,
// 17-20 pinky (each finger listed from its base to its tip).

struct HandInfo {
	bool found = false;
	int fingerCount = 0;
	float confidence = 0.f;                 // hand presence score from the landmark model
	std::vector<cv::Point2f> landmarks;     // 21 keypoints in frame coordinates
	std::array<bool, 5> fingerUp{};         // thumb, index, middle, ring, pinky
	std::vector<cv::Point> fingertips;      // tips of the raised fingers
	cv::Rect box;                           // bounding box of the landmarks
};

class FingerDetector {
public:
	// modelDir must contain palm_detection_mediapipe_2023feb.onnx and
	// handpose_estimation_mediapipe_2023feb.onnx (CMake downloads them into model/)
	explicit FingerDetector(const std::string& modelDir = CTRL_MODEL_DIR,
	                        float palmScoreThreshold = 0.5f,
	                        float handScoreThreshold = 0.7f);

	bool IsReady() const { return ready; }

	HandInfo Detect(const cv::Mat& frame);

	// Draws the hand skeleton, raised fingertips and finger count onto frame
	static void Draw(cv::Mat& frame, const HandInfo& hand);

private:
	// The 7 points the hand crop is derived from: wrist, the 4 finger bases and
	// 2 thumb joints. Comes from the palm detector or from the previous landmarks.
	using PalmPoints = std::array<cv::Point2f, 7>;

	bool DetectPalm(const cv::Mat& frame, PalmPoints& palm);
	bool EstimateLandmarks(const cv::Mat& frame, const PalmPoints& palm, HandInfo& hand);
	static void CountFingers(HandInfo& hand);

	cv::dnn::Net palmNet;
	cv::dnn::Net handNet;
	std::vector<std::string> palmOutputs;
	std::vector<std::string> handOutputs;
	std::vector<cv::Point2f> anchors; // palm detector anchor centers, normalized
	bool ready = false;

	float palmThreshold;
	float handThreshold;

	bool tracking = false; // previous frame had a hand, reuse its landmarks
	PalmPoints lastPalm;
};
