#pragma once

struct _XDisplay;
using Key = unsigned long;

class Input {
public:
	Input();
	~Input();

	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

	// True if the X display was opened successfully
	bool IsReady() const;

	// Press and release key
	void PressKey(Key keysym);

private:
	_XDisplay* display;
};