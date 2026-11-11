#pragma once

#include <QImage>
#include <QString>
#include <QThread>

// Runs in the background so the window doesn't freeze.
// It reads the webcam, finds the hand, moves the mouse and presses space.
// The window only shows the pictures it sends.

class CameraThread : public QThread {
	Q_OBJECT

public:
	~CameraThread();

signals:
	// A new camera picture, with the hand drawn on it
	void FrameReady(const QImage& image);

	// Something could not be opened, the thread has stopped
	void Failed(const QString& message);

protected:
	void run() override;
};
