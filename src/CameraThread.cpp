#include "CameraThread.hpp"

#include "Capture.hpp"
#include "FingerDetector.hpp"
#include "Input.hpp"
#include "MouseController.hpp"

const Key SPACE_KEY = 0x20;  // X11 code of the space bar
const int SPACE_FRAMES = 5;  // how many frames the open hand must be shown

CameraThread::~CameraThread() {
	// Ask the loop in run() to stop and wait for it
	requestInterruption();
	wait();
}

void CameraThread::run() {
	Capture capture(640, 480);
	if (!capture.ConnectWebcam()) {
		emit Failed("Webcam not found (/dev/video0).");
		return;
	}

	FingerDetector fingerDetector;
	if (!fingerDetector.IsReady()) {
		emit Failed("Could not load the hand models. Run cmake again to download them.");
		return;
	}

	Input input;
	if (!input.IsReady()) {
		emit Failed("Cannot open the X display.");
		return;
	}

	MouseController mouse(input);
	int openHandFrames = 0; // how many frames in a row we saw 5 fingers

	while (!isInterruptionRequested()) {
		cv::Mat frame = capture.GetFrame();
		if (frame.empty()) {
			msleep(10);
			continue;
		}

		// Flip it like a mirror, so moving the hand right moves the cursor right
		cv::flip(frame, frame, 1);

		HandInfo hand = fingerDetector.Detect(frame);

		// Pointing finger moves the mouse
		mouse.Update(hand, frame.size());

		// Open hand presses space, once each time it is shown
		if (hand.found && hand.fingerCount == 5) {
			openHandFrames++;
		} else {
			openHandFrames = 0;
		}
		if (openHandFrames == SPACE_FRAMES) {
			input.PressKey(SPACE_KEY);
		}

		// Draw the hand, and a box around the part of the picture used as the screen
		FingerDetector::Draw(frame, hand);
		cv::Rect2f area = MouseController::ScreenArea();
		cv::Rect box(area.x * frame.cols, area.y * frame.rows,
		             area.width * frame.cols, area.height * frame.rows);
		if (mouse.IsActive()) {
			cv::rectangle(frame, box, cv::Scalar(0, 255, 255), 2); // yellow while pointing
		} else {
			cv::rectangle(frame, box, cv::Scalar(128, 128, 128), 2);
		}

		// Send the picture to the window. copy() is needed because
		// `frame` is reused for the next picture.
		QImage image(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);
		emit FrameReady(image.copy());
	}

	// Don't leave the mouse button pressed when we stop
	mouse.Stop();
}
