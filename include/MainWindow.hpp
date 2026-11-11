#pragma once

#include <QImage>
#include <QList>
#include <QMainWindow>

#include "CameraThread.hpp"
#include "Detector.hpp"
#include "Input.hpp"

class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class KeyCaptureButton;

// Main window: live camera view on the left, the user's gesture -> key
// bindings on the right. Owns the camera thread, the Detector and the Input.

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

private slots:
	void OnFrame(const QImage& image, const HandInfo& hand);
	void OnCameraFailed(const QString& message);
	void AddBinding();
	void RemoveSelectedBinding();

private:
	// One row of the bindings table
	struct Binding {
		bool exact;  // true: these exact fingers, false: any `value` fingers
		int value;   // finger count (0..5), or finger mask (bit 0 = thumb) when exact
		Key key;
	};

	QWidget* BuildBindingsPanel();
	void ApplyBindings(); // pushes `bindings` into the detector, the table and the settings
	void LoadBindings();
	static QString GestureText(const Binding& b);

	CameraThread camera;
	Detector detector;
	Input input;
	QList<Binding> bindings;

	QLabel* cameraView;
	QLabel* gestureLabel;
	QCheckBox* enableInput;
	QTableWidget* table;
	QComboBox* gestureKind;
	QStackedWidget* gestureInputs;
	QSpinBox* countInput;
	QCheckBox* fingerInputs[5];
	KeyCaptureButton* keyInput;
};
