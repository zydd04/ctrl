#include <opencv2/opencv.hpp>
#include <X11/keysym.h>   // XK_space etc. (include AFTER OpenCV)
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Capture.hpp"
#include "Detector.hpp"
#include "FingerDetector.hpp"
#include "Input.hpp"

const double THRESHOLD = 0.5;
const auto PRESS_COOLDOWN = std::chrono::milliseconds(300); // no spam

static std::atomic<bool> running{true};

static void OnSignal(int) { running = false; } // Ctrl+C stops the loop cleanly

int main() {
	std::signal(SIGINT, OnSignal);
	std::signal(SIGTERM, OnSignal);

	Capture capture(640, 480); // must be larger than your template image
	if (!capture.ConnectWebcam()) {
		std::cerr << "Webcam not found (/dev/video0). Check permissions: are you in the 'video' group?\n";
		return 1;
	}

	Input input;
	if (!input.IsReady()) {
		return 1;
	}

	// template image path -> key to press
	std::unordered_map<std::string, Key> gestureToKey = {
		{"/home/ziyad/Desktop/projects/ctrl/img.png", XK_space}
	};
	Detector detector(gestureToKey, THRESHOLD);
	FingerDetector fingerDetector;

	const std::string CAMERA_WINDOW = "ctrl - camera";
	const std::string MASK_WINDOW = "ctrl - skin mask";
	cv::namedWindow(CAMERA_WINDOW, cv::WINDOW_AUTOSIZE);
	cv::namedWindow(MASK_WINDOW, cv::WINDOW_AUTOSIZE);

	auto lastPress = std::chrono::steady_clock::now();
	while (running) {
		cv::Mat frame = capture.GetFrame();
		if (frame.empty()) {
			continue;
		}

		// Mirrored copy for finger detection/display so it feels natural;
		// the template detector keeps using the original frame
		cv::Mat mirrored;
		cv::flip(frame, mirrored, 1);
		HandInfo hand = fingerDetector.Detect(mirrored);

		Key key = detector.Detect(frame);
		if (key != 0) {
			auto now = std::chrono::steady_clock::now();
			if (now - lastPress > PRESS_COOLDOWN) {
				input.PressKey(key);
				lastPress = now;
			}
		}

		FingerDetector::Draw(mirrored, hand);
		cv::imshow(CAMERA_WINDOW, mirrored);
		if (!hand.mask.empty()) {
			cv::imshow(MASK_WINDOW, hand.mask);
		}

		int k = cv::waitKey(1) & 0xFF; // also pumps the GUI event loop
		if (k == 'q' || k == 27) {     // q or Esc quits
			running = false;
		}
	}
	cv::destroyAllWindows();
	return 0;
}