#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QShortcut>

#include "CameraThread.hpp"

//Open hand (5 fingers) = space
//1 finger = move mouse 
//index close to thumb = click

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	QLabel view("Starting camera...");
	view.setWindowTitle("ctrl");
	view.setAlignment(Qt::AlignCenter);
	view.setFixedSize(640, 480);

	// hint at the bottom 
	QLabel hint("q (quit), p (pause)", &view);
	hint.setAlignment(Qt::AlignCenter);
	hint.setStyleSheet("color: white; background-color: rgba(0, 0, 0, 120); font-size: 11px;");
	hint.setGeometry(0, view.height() - 20, view.width(), 20);

	view.show();

	CameraThread camera;

	// Feed pictures from CameraThread
	QObject::connect(&camera, &CameraThread::FrameReady, &view, [&view](const QImage& image) {
		view.setPixmap(QPixmap::fromImage(image));
	});

	// Error handling
	QObject::connect(&camera, &CameraThread::Failed, &view, &QLabel::setText);

	// P to pause or resumes, Q quit
	QShortcut pauseKey(QKeySequence(Qt::Key_P), &view);
	QObject::connect(&pauseKey, &QShortcut::activated, &camera, &CameraThread::TogglePause);
	QShortcut quitKey(QKeySequence(Qt::Key_Q), &view);
	QObject::connect(&quitKey, &QShortcut::activated, &app, &QApplication::quit);

	camera.start();
	return app.exec();
}
