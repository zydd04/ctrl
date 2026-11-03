#include "Input.hpp"

#include <iostream>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

void DisplayCloser::operator()(_XDisplay* d) const {
    XCloseDisplay(d);
}

Input::Input() {
    display.reset(XOpenDisplay(nullptr));
    if (!display) {
        std::cerr << "Input: cannot open X display (is DISPLAY set?)\n";
    }
}

bool Input::IsReady() const {
    return display != nullptr;
}

void Input::PressKey(Key keysym) {
    if (!display || keysym == 0) {
        return;
    }
    KeyCode keycode = XKeysymToKeycode(display.get(), static_cast<KeySym>(keysym));
    if (keycode == 0) {
        return;
    }
    XTestFakeKeyEvent(display.get(), keycode, True, 0);
    XTestFakeKeyEvent(display.get(), keycode, False, 0);
    XFlush(display.get());
}