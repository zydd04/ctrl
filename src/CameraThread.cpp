#include "CameraThread.hpp"

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

void CameraThread::TogglePause() {
	paused = !paused;
}

void CameraThread::run() {
	cv::VideoCapture capture(0, cv::CAP_V4L2); // Linux webcam (/dev/video0)
	if (!capture.isOpened()) {
		emit Failed("Webcam not found (/dev/video0).");
		return;
	}
	capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
	capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

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

	cv::Mat frame;
	while (!isInterruptionRequested()) {
		if (!capture.read(frame)) {
			msleep(10);
			continue;
		}

		// Flip it like a mirror, so moving the hand right moves the cursor right
		cv::flip(frame, frame, 1);

		if (paused) {
			// Let go of the mouse and only show the picture
			mouse.Stop();
			openHandFrames = 0;
			cv::putText(frame, "PAUSED (P to resume)", cv::Point(20, 40),
			            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
		} else {
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

			// Draw the hand, and a box around the part of the picture used as
			// the screen (yellow while pointing)
			FingerDetector::Draw(frame, hand);
			cv::Rect2f area = MouseController::ScreenArea();
			cv::Rect box(area.x * frame.cols, area.y * frame.rows,
			             area.width * frame.cols, area.height * frame.rows);
			cv::Scalar color = mouse.IsActive() ? cv::Scalar(0, 255, 255) : cv::Scalar(128, 128, 128);
			cv::rectangle(frame, box, color, 2);
		}

		// Send the picture to the window. copy() is needed because
		// `frame` is reused for the next picture.
		QImage image(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);
		emit FrameReady(image.copy());
	}

	// Don't leave the mouse button pressed when we stop
	mouse.Stop();
}
