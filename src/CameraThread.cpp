#include "CameraThread.hpp"

#include "Capture.hpp"

CameraThread::~CameraThread() {
	Stop();
}

void CameraThread::Stop() {
	requestInterruption();
	wait();
}

void CameraThread::run() {
	Capture capture(640, 480);
	if (!capture.ConnectWebcam()) {
		emit Failed(tr("Webcam not found (/dev/video0). Is it plugged in, and are you "
		               "in the 'video' group?"));
		return;
	}

	FingerDetector fingerDetector;
	if (!fingerDetector.IsReady()) {
		emit Failed(tr("Could not load the hand models from %1. Re-run cmake to download them.")
		            .arg(CTRL_MODEL_DIR));
		return;
	}

	while (!isInterruptionRequested()) {
		cv::Mat frame = capture.GetFrame();
		if (frame.empty()) {
			msleep(10);
			continue;
		}

		// Mirrored so the display feels natural
		cv::Mat mirrored;
		cv::flip(frame, mirrored, 1);
		HandInfo hand = fingerDetector.Detect(mirrored);
		FingerDetector::Draw(mirrored, hand);

		// copy(): the QImage must own its pixels once `mirrored` goes away
		QImage image(mirrored.data, mirrored.cols, mirrored.rows,
		             static_cast<qsizetype>(mirrored.step), QImage::Format_BGR888);
		emit FrameReady(image.copy(), hand);
	}
}
