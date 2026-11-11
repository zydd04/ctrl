#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QShortcut>

#include "CameraThread.hpp"

// ctrl: control the computer with your hand.
//   point with the index finger -> move the mouse
//   touch thumb and index       -> click (keep them together to drag)
//   open hand (5 fingers)       -> press space
// Keys (while the ctrl window is focused):
//   P -> pause / resume the hand control
//   Q -> quit

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	// The window is just a label that shows the camera picture
	QLabel view("Starting camera...");
	view.setWindowTitle("ctrl");
	view.setAlignment(Qt::AlignCenter);
	view.setFixedSize(640, 480);

	// Small text at the bottom of the window with the keys
	QLabel hint("q (quit), p (pause)", &view);
	hint.setAlignment(Qt::AlignCenter);
	hint.setStyleSheet("color: white; background-color: rgba(0, 0, 0, 120); font-size: 11px;");
	hint.setGeometry(0, view.height() - 20, view.width(), 20);

	view.show();

	CameraThread camera;

	// Show every new picture from the camera thread
	QObject::connect(&camera, &CameraThread::FrameReady, &view, [&view](const QImage& image) {
		view.setPixmap(QPixmap::fromImage(image));
	});

	// Show the error text if the camera thread could not start
	QObject::connect(&camera, &CameraThread::Failed, &view, &QLabel::setText);

	// P pauses or resumes, Q quits
	QShortcut pauseKey(QKeySequence(Qt::Key_P), &view);
	QObject::connect(&pauseKey, &QShortcut::activated, &camera, &CameraThread::TogglePause);
	QShortcut quitKey(QKeySequence(Qt::Key_Q), &view);
	QObject::connect(&quitKey, &QShortcut::activated, &app, &QApplication::quit);

	camera.start();
	return app.exec();
}
