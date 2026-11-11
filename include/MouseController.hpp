#pragma once

#include <opencv2/opencv.hpp>
#include "FingerDetector.hpp"
#include "Input.hpp"

// Uses the hand as a mouse

class MouseController {
public:
	MouseController(Input& input);

	void Update(const HandInfo& hand, cv::Size frameSize);
	void Stop();
	bool IsActive() const;

	// Part of the picture used as the screen (0 to 1)
	static cv::Rect2f ScreenArea();

private:
	Input& input;
	bool active = false;
	bool pressed = false;
	float cursorX = 0; // 0 to 1 of the screen
	float cursorY = 0;
};
