#pragma once

struct _XDisplay;

// Same type as X11's KeySym (e.g. XK_space from <X11/keysym.h>)
using Key = unsigned long;

class Input {
public:
	Input();

	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

	// True if the X display was opened successfully
	bool IsReady() const;

	// Press and release key
	void PressKey(Key keysym);

private:
	_XDisplay* display;
};