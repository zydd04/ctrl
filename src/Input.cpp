#include "Input.hpp"

#include <iostream>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

Input::Input() {
	display = XOpenDisplay(nullptr);
	if (!display) {
		std::cerr << "Input: cannot open X display (is DISPLAY set?)\n";
	}
}

Input::~Input() {
	if (display) {
		XCloseDisplay(display);
	}
}

bool Input::IsReady() const {
	return display != nullptr;
}

void Input::PressKey(Key keysym) {
	if (!display || keysym == 0) {
		return;
	}
	KeyCode keycode = XKeysymToKeycode(display, static_cast<KeySym>(keysym));
	if (keycode == 0) {
		return; 
	}
	XTestFakeKeyEvent(display, keycode, True, 0);  // key down
	XTestFakeKeyEvent(display, keycode, False, 0); // key up
	XFlush(display);
}