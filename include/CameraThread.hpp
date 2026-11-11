#pragma once

#include <atomic>

#include <QImage>
#include <QString>
#include <QThread>

// Camera loop in the background so the window doesn't freeze

class CameraThread : public QThread {
	Q_OBJECT

public:
	~CameraThread();

	void TogglePause();

signals:
	void FrameReady(const QImage& image);
	void Failed(const QString& message);

protected:
	void run() override;

private:
	std::atomic<bool> paused{false};
};
