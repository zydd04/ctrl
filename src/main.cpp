#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QShortcut>
#include <QSlider>

#include "CameraThread.hpp"
#include "MouseController.hpp"

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
	
	// controls at the top
	QLabel speedText("speed", &view);
	QLabel pinchText("pinch", &view);
	speedText.setStyleSheet("color: white; font-size: 11px;");
	pinchText.setStyleSheet("color: white; font-size: 11px;");
	speedText.setGeometry(400, 2, 110, 14);
	pinchText.setGeometry(520, 2, 110, 14);

	QSlider speedSlider(Qt::Horizontal, &view);
	QSlider pinchSlider(Qt::Horizontal, &view);
	speedSlider.setGeometry(400, 16, 110, 18);
	pinchSlider.setGeometry(520, 16, 110, 18);
	speedSlider.setRange(1, 50);
	pinchSlider.setRange(10, 60);
	speedSlider.setValue(followSpeed * 100);
	pinchSlider.setValue(pinchDistance * 100);

	// slider value / 100 = setting
	QObject::connect(&speedSlider, &QSlider::valueChanged, [](int value) { followSpeed = value / 100.0f; });
	QObject::connect(&pinchSlider, &QSlider::valueChanged, [](int value) { pinchDistance = value / 100.0f; });

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
