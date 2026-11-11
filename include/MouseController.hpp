#pragma once

#include <opencv2/opencv.hpp>
#include "FingerDetector.hpp"
#include "Input.hpp"

// Uses the hand as a mouse:
//   - point with the index finger (middle, ring and pinky closed) to move the cursor
//   - touch thumb and index together to click, keep them together to drag

class MouseController {
public:
	MouseController(Input& input);

	// Call this for every camera frame
	void Update(const HandInfo& hand, cv::Size frameSize);

	// Stops pointing and lets go of the mouse button
	void Stop();

	// True while the hand is moving the cursor
	bool IsActive() const;

	// The part of the camera picture that is used as the screen.
	// It is smaller than the picture so the hand can reach every corner.
	// Values are fractions of the picture size.
	static cv::Rect2f ScreenArea();

private:
	Input& input;
	bool active = false;  // the hand is pointing
	bool pressed = false; // the mouse button is down
	float cursorX = 0;    // cursor position, 0..1 of the screen
	float cursorY = 0;
};
