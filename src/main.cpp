#include <QApplication>

#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	QApplication::setOrganizationName("ctrl"); // where QSettings stores the bindings
	QApplication::setApplicationName("ctrl");

	qRegisterMetaType<HandInfo>(); // lets HandInfo cross from the camera thread to the UI

	MainWindow window;
	window.show();
	return app.exec();
}
