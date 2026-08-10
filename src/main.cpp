#include <iostream>
#include <opencv2/opencv.hpp>
#include "Capture.hpp"
#include "Detector.hpp"
#include "Input.hpp"

int main() {
	Capture cap = Capture(200, 200);
	Detector Detector("C:/Users/ziyad/OneDrive/Desktop/projects/ctrl/data/open.png", 0.9);
	Input input;

	while(true) {
		cv::Mat frame = cap.GetFrame();
		if (frame.empty()) {
			std::cout << "Frame is Empty.\n";
			continue;
		}
		WORD key = Detector.Detect(frame);
		if (key != 0) {
			input.PressKey(key);
			std::cout << "Match, pressing space" << "\n";
		}
		cv::imshow("ctrl", frame);
		if (cv::waitKey(1) == 27) {
			break;  
			cv::destroyAllWindows();
		}
	}
	return 0;
}