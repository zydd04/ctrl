#pragma once

#include <QImage>
#include <QMetaType>
#include <QString>
#include <QThread>

#include "FingerDetector.hpp"

// Background thread that reads the webcam, finds the hand in every frame and
// hands the result to the UI. Keeps the slow camera/model work off the GUI thread.

class CameraThread : public QThread {
	Q_OBJECT

public:
	using QThread::QThread;
	~CameraThread() override;

	// Asks the loop to finish and waits for it
	void Stop();

signals:
	// image is the mirrored camera frame with the hand skeleton drawn on it
	void FrameReady(const QImage& image, const HandInfo& hand);

	// The camera or the hand models could not be opened; the thread has ended
	void Failed(const QString& message);

protected:
	void run() override;
};

Q_DECLARE_METATYPE(HandInfo)
