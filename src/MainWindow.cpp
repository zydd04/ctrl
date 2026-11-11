#include "MainWindow.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

const char* const FINGER_NAMES[5] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
const Key DEFAULT_KEY = 0x20; // space, bound to an open hand on first start

QString CountText(int count) {
	return count == 1 ? QObject::tr("1 finger") : QObject::tr("%1 fingers").arg(count);
}

QString KeyText(Key key) {
	return QString::fromStdString(Input::KeyName(key));
}

Fingers MaskToFingers(int mask) {
	Fingers fingers{};
	for (int f = 0; f < 5; ++f) {
		fingers[f] = mask & (1 << f);
	}
	return fingers;
}

} // namespace

// Button that records the next key pressed on the keyboard: click it, then
// press the key that the gesture should send.
class KeyCaptureButton : public QPushButton {
public:
	explicit KeyCaptureButton(QWidget* parent = nullptr) : QPushButton(parent) {
		setCheckable(true);
		UpdateText();
		connect(this, &QPushButton::toggled, this, [this] { UpdateText(); });
	}

	Key ChosenKey() const { return key; }

protected:
	void keyPressEvent(QKeyEvent* event) override {
		if (!isChecked()) {
			QPushButton::keyPressEvent(event);
			return;
		}
		// On X11 the native virtual key is the X keysym, which is what Input sends
		if (Key pressed = event->nativeVirtualKey()) {
			key = pressed;
			swallowRelease = true;
			setChecked(false);
		}
	}

	void keyReleaseEvent(QKeyEvent* event) override {
		// Releasing a captured Space/Enter must not click the button again
		if (swallowRelease) {
			swallowRelease = false;
			return;
		}
		QPushButton::keyReleaseEvent(event);
	}

	void focusOutEvent(QFocusEvent* event) override {
		setChecked(false);
		QPushButton::focusOutEvent(event);
	}

private:
	void UpdateText() {
		if (isChecked()) {
			setText(tr("Press a key..."));
		} else {
			setText(key ? KeyText(key) : tr("Click to choose a key"));
		}
	}

	Key key = 0;
	bool swallowRelease = false;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	setWindowTitle("ctrl");

	cameraView = new QLabel(tr("Starting camera..."));
	cameraView->setAlignment(Qt::AlignCenter);
	cameraView->setMinimumSize(640, 480);
	cameraView->setStyleSheet("background: black; color: white;");

	auto* central = new QWidget;
	auto* layout = new QHBoxLayout(central);
	layout->addWidget(cameraView, 1);
	layout->addWidget(BuildBindingsPanel());
	setCentralWidget(central);

	LoadBindings();
	ApplyBindings();

	if (!input.IsReady()) {
		enableInput->setChecked(false);
		enableInput->setEnabled(false);
		statusBar()->showMessage(tr("Cannot open the X display: key presses are disabled"));
	}

	connect(&camera, &CameraThread::FrameReady, this, &MainWindow::OnFrame);
	connect(&camera, &CameraThread::Failed, this, &MainWindow::OnCameraFailed);
	camera.start();
}

MainWindow::~MainWindow() {
	camera.Stop();
}

QWidget* MainWindow::BuildBindingsPanel() {
	auto* panel = new QWidget;
	panel->setFixedWidth(340);
	auto* layout = new QVBoxLayout(panel);
	layout->setContentsMargins(0, 0, 0, 0);

	gestureLabel = new QLabel(tr("No hand"));
	QFont big = gestureLabel->font();
	big.setPointSize(big.pointSize() + 4);
	big.setBold(true);
	gestureLabel->setFont(big);
	layout->addWidget(gestureLabel);

	enableInput = new QCheckBox(tr("Send key presses"));
	enableInput->setChecked(true);
	layout->addWidget(enableInput);

	// Existing bindings
	table = new QTableWidget(0, 2);
	table->setHorizontalHeaderLabels({tr("Gesture"), tr("Key")});
	table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	table->verticalHeader()->hide();
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	layout->addWidget(table, 1);

	auto* removeButton = new QPushButton(tr("Remove selected"));
	connect(removeButton, &QPushButton::clicked, this, &MainWindow::RemoveSelectedBinding);
	layout->addWidget(removeButton);

	// New binding
	auto* addBox = new QGroupBox(tr("New binding"));
	auto* form = new QFormLayout(addBox);

	gestureKind = new QComboBox;
	gestureKind->addItems({tr("Number of fingers"), tr("Specific fingers")});
	form->addRow(tr("Gesture"), gestureKind);

	countInput = new QSpinBox;
	countInput->setRange(0, 5);
	countInput->setValue(1);

	auto* fingersRow = new QWidget;
	auto* fingersLayout = new QVBoxLayout(fingersRow);
	fingersLayout->setContentsMargins(0, 0, 0, 0);
	for (int f = 0; f < 5; ++f) {
		fingerInputs[f] = new QCheckBox(tr(FINGER_NAMES[f]));
		fingersLayout->addWidget(fingerInputs[f]);
	}

	gestureInputs = new QStackedWidget;
	gestureInputs->addWidget(countInput);
	gestureInputs->addWidget(fingersRow);
	// Show one page and let only that page decide the height, otherwise the
	// number box is stretched to the height of the five checkboxes
	auto showGesturePage = [this](int index) {
		for (int i = 0; i < gestureInputs->count(); ++i) {
			auto policy = i == index ? QSizePolicy::Preferred : QSizePolicy::Ignored;
			gestureInputs->widget(i)->setSizePolicy(QSizePolicy::Preferred, policy);
		}
		gestureInputs->setCurrentIndex(index);
	};
	connect(gestureKind, &QComboBox::currentIndexChanged, this, showGesturePage);
	showGesturePage(0);
	form->addRow(tr("Raised"), gestureInputs);

	keyInput = new KeyCaptureButton;
	form->addRow(tr("Key"), keyInput);

	auto* addButton = new QPushButton(tr("Add binding"));
	connect(addButton, &QPushButton::clicked, this, &MainWindow::AddBinding);
	form->addRow(addButton);

	layout->addWidget(addBox);
	return panel;
}

QString MainWindow::GestureText(const Binding& b) {
	if (!b.exact) {
		return b.value == 0 ? tr("Fist (0 fingers)") : CountText(b.value);
	}
	QStringList names;
	for (int f = 0; f < 5; ++f) {
		if (b.value & (1 << f)) {
			names << tr(FINGER_NAMES[f]);
		}
	}
	return names.isEmpty() ? tr("Fist (no fingers)") : names.join(" + ");
}

void MainWindow::AddBinding() {
	Key key = keyInput->ChosenKey();
	if (key == 0) {
		statusBar()->showMessage(tr("Choose a key first"), 3000);
		return;
	}

	Binding binding{gestureKind->currentIndex() == 1, 0, key};
	if (binding.exact) {
		for (int f = 0; f < 5; ++f) {
			if (fingerInputs[f]->isChecked()) {
				binding.value |= 1 << f;
			}
		}
	} else {
		binding.value = countInput->value();
	}

	// A gesture has one key: binding it again replaces the old key
	for (Binding& existing : bindings) {
		if (existing.exact == binding.exact && existing.value == binding.value) {
			existing.key = key;
			ApplyBindings();
			return;
		}
	}
	bindings.append(binding);
	ApplyBindings();
}

void MainWindow::RemoveSelectedBinding() {
	int row = table->currentRow();
	if (row < 0 || row >= bindings.size()) {
		return;
	}
	bindings.removeAt(row);
	ApplyBindings();
}

void MainWindow::ApplyBindings() {
	detector.ClearBindings();
	table->setRowCount(static_cast<int>(bindings.size()));
	QStringList saved;

	for (int row = 0; row < bindings.size(); ++row) {
		const Binding& b = bindings[row];
		if (b.exact) {
			detector.BindFingers(MaskToFingers(b.value), b.key);
		} else {
			detector.BindCount(b.value, b.key);
		}
		table->setItem(row, 0, new QTableWidgetItem(GestureText(b)));
		table->setItem(row, 1, new QTableWidgetItem(KeyText(b.key)));
		saved << QString("%1:%2:%3").arg(b.exact ? "fingers" : "count").arg(b.value).arg(b.key);
	}
	QSettings().setValue("bindings", saved);
}

void MainWindow::LoadBindings() {
	QSettings settings;
	if (!settings.contains("bindings")) {
		bindings = {{false, 5, DEFAULT_KEY}}; // first start: open hand -> space
		return;
	}
	const QStringList saved = settings.value("bindings").toStringList();
	for (const QString& entry : saved) {
		const QStringList parts = entry.split(':');
		if (parts.size() != 3) {
			continue;
		}
		Binding b{parts[0] == "fingers", parts[1].toInt(), parts[2].toULong()};
		bool valid = b.key != 0 && b.value >= 0 && b.value <= (b.exact ? 31 : 5);
		if (valid) {
			bindings.append(b);
		}
	}
}

void MainWindow::OnFrame(const QImage& image, const HandInfo& hand) {
	cameraView->setPixmap(QPixmap::fromImage(image).scaled(
		cameraView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

	// Live preview of what the current hand is bound to
	if (!hand.found) {
		gestureLabel->setText(tr("No hand"));
	} else {
		Key bound = detector.KeyFor(hand);
		gestureLabel->setText(CountText(hand.fingerCount) + "  ->  " +
		                      (bound ? KeyText(bound) : tr("not bound")));
	}

	// Detect() must run every frame so it can time how long a gesture is held
	Key key = detector.Detect(hand);
	if (key != 0 && enableInput->isChecked()) {
		input.PressKey(key);
		statusBar()->showMessage(tr("Pressed %1").arg(KeyText(key)), 2000);
	}
}

void MainWindow::OnCameraFailed(const QString& message) {
	cameraView->setText(message);
	cameraView->setWordWrap(true);
	statusBar()->showMessage(tr("Camera stopped"));
}
