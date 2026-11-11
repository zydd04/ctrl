#include "MouseController.hpp"

#include <algorithm>
#include <cmath>

// How close thumb and index must be to click, compared to the palm size.
// It has to open wider than it closed, so the click doesn't flicker.
const float PINCH_CLOSE = 0.25f;
const float PINCH_OPEN = 0.40f;

// How fast the cursor follows the finger (0..1).
// Smaller = smoother but slower, bigger = faster but shakier.
const float FOLLOW_SPEED = 0.35f;

static float Distance(cv::Point2f a, cv::Point2f b) {
	return std::hypot(a.x - b.x, a.y - b.y);
}

MouseController::MouseController(Input& input) : input(input) {}

bool MouseController::IsActive() const {
	return active;
}

cv::Rect2f MouseController::ScreenArea() {
	// x, y, width, height
	return cv::Rect2f(0.20f, 0.12f, 0.60f, 0.55f);
}

void MouseController::Stop() {
	if (pressed) {
		input.MouseUp();
	}
	pressed = false;
	active = false;
}

void MouseController::Update(const HandInfo& hand, cv::Size frameSize) {
	if (!hand.found) {
		Stop();
		return;
	}

	cv::Point2f wrist = hand.landmarks[0];
	cv::Point2f thumbTip = hand.landmarks[4];
	cv::Point2f indexTip = hand.landmarks[8];
	cv::Point2f middleBase = hand.landmarks[9];

	// Are thumb and index touching?
	float palmSize = std::max(1.0f, Distance(wrist, middleBase));
	float pinch = Distance(thumbTip, indexTip) / palmSize;
	bool pinched;
	if (pressed) {
		pinched = pinch < PINCH_OPEN;
	} else {
		pinched = pinch < PINCH_CLOSE;
	}

	// Pointing = index raised, and middle, ring and pinky closed.
	// A pinch also counts, because pinching can bend the index finger.
	bool othersClosed = !hand.fingerUp[2] && !hand.fingerUp[3] && !hand.fingerUp[4];
	bool pointing = othersClosed && (hand.fingerUp[1] || pinched);
	if (!pointing) {
		Stop();
		return;
	}

	// Where the fingertip is inside the screen area, as 0..1
	cv::Rect2f area = ScreenArea();
	float x = (indexTip.x / frameSize.width - area.x) / area.width;
	float y = (indexTip.y / frameSize.height - area.y) / area.height;
	x = std::clamp(x, 0.0f, 1.0f);
	y = std::clamp(y, 0.0f, 1.0f);

	// Move the cursor part of the way to the fingertip each frame,
	// so a shaky hand doesn't make a shaky cursor
	if (!active) {
		cursorX = x;
		cursorY = y;
		active = true;
	} else {
		cursorX += (x - cursorX) * FOLLOW_SPEED;
		cursorY += (y - cursorY) * FOLLOW_SPEED;
	}
	input.MoveMouse(cursorX, cursorY);

	// Click
	if (pinched && !pressed) {
		input.MouseDown();
		pressed = true;
	} else if (!pinched && pressed) {
		input.MouseUp();
		pressed = false;
	}
}
