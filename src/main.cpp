#include <QApplication>
#include <QLabel>
#include <QPixmap>

#include "CameraThread.hpp"

// ctrl: control the computer with your hand.
//   point with the index finger -> move the mouse
//   touch thumb and index       -> click (keep them together to drag)
//   open hand (5 fingers)       -> press space

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	// The window is just a label that shows the camera picture
	QLabel view("Starting camera...");
	view.setWindowTitle("ctrl");
	view.setAlignment(Qt::AlignCenter);
	view.setFixedSize(640, 480);
	view.show();

	CameraThread camera;

	// Show every new picture from the camera thread
	QObject::connect(&camera, &CameraThread::FrameReady, &view, [&view](const QImage& image) {
		view.setPixmap(QPixmap::fromImage(image));
	});

	// Show the error text if the camera thread could not start
	QObject::connect(&camera, &CameraThread::Failed, &view, &QLabel::setText);

	camera.start();
	return app.exec();
}
