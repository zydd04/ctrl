#include <opencv2/opencv.hpp>
#include <X11/keysym.h>   // XK_space etc. (include AFTER OpenCV)
#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

#include "Capture.hpp"
#include "Detector.hpp"
#include "FingerDetector.hpp"
#include "Input.hpp"

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

	// Example bindings until they come from the UI
	Detector detector;
	detector.BindCount(5, XK_space);                                  // open hand
	detector.BindFingers({false, true, true, false, false}, XK_Right); // index + middle
	FingerDetector fingerDetector;
	if (!fingerDetector.IsReady()) {
		return 1;
	}

	const std::string CAMERA_WINDOW = "ctrl - camera";
	cv::namedWindow(CAMERA_WINDOW, cv::WINDOW_AUTOSIZE);

	while (running) {
		cv::Mat frame = capture.GetFrame();
		if (frame.empty()) {
			continue;
		}

		// Mirrored so the display feels natural
		cv::Mat mirrored;
		cv::flip(frame, mirrored, 1);
		HandInfo hand = fingerDetector.Detect(mirrored);

		Key key = detector.Detect(hand);
		if (key != 0) {
			input.PressKey(key);
		}

		cv::Mat display = mirrored.clone();
		FingerDetector::Draw(display, hand);

		cv::putText(display, "q: quit", cv::Point(10, display.rows - 12),
		            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 165, 255), 1);
		cv::imshow(CAMERA_WINDOW, display);

		int k = cv::waitKey(1) & 0xFF; // also pumps the GUI event loop
		if (k == 'q' || k == 27) {     // q or Esc quits
			running = false;
		}
	}
	cv::destroyAllWindows();
	return 0;
}