#include "MouseController.hpp"

#include <algorithm>

// Thumb to index distance to click / release, compared to palm size
const float PINCH_CLOSE = 0.25f;
const float PINCH_OPEN = 0.40f;

// Cursor follow speed, smaller = smoother
const float FOLLOW_SPEED = 0.10f;

static float Distance(cv::Point2f a, cv::Point2f b) {
	return cv::norm(a - b);
}

MouseController::MouseController(Input& input) : input(input) {}

bool MouseController::IsActive() const {
	return active;
}

cv::Rect2f MouseController::ScreenArea() {
	// x, y, width, height
	return cv::Rect2f(0.10f, 0.08f, 0.80f, 0.70f);
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

	// Thumb touching index?
	float palmSize = std::max(1.0f, Distance(wrist, middleBase));
	float pinch = Distance(thumbTip, indexTip) / palmSize;
	bool pinched = pinch < (pressed ? PINCH_OPEN : PINCH_CLOSE);

	// Pointing = index up, others closed (a pinch counts too)
	bool othersClosed = !hand.fingerUp[2] && !hand.fingerUp[3] && !hand.fingerUp[4];
	bool pointing = othersClosed && (hand.fingerUp[1] || pinched);
	if (!pointing) {
		Stop();
		return;
	}

	// Fingertip inside the screen area, 0 to 1
	cv::Rect2f area = ScreenArea();
	float x = (indexTip.x / frameSize.width - area.x) / area.width;
	float y = (indexTip.y / frameSize.height - area.y) / area.height;
	x = std::clamp(x, 0.0f, 1.0f);
	y = std::clamp(y, 0.0f, 1.0f);

	// Smooth the cursor
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
