#include "Input.hpp"

#include <iostream>
#include <X11/Xlib.h>
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
    if (!display) {
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

void Input::MoveMouse(float x, float y) {
    if (!display) {
        return;
    }
    int screen = DefaultScreen(display.get());
    int px = static_cast<int>(x * (DisplayWidth(display.get(), screen) - 1));
    int py = static_cast<int>(y * (DisplayHeight(display.get(), screen) - 1));
    XTestFakeMotionEvent(display.get(), screen, px, py, 0);
    XFlush(display.get());
}

void Input::MouseDown() {
    if (display) {
        XTestFakeButtonEvent(display.get(), Button1, True, 0);
        XFlush(display.get());
    }
}

void Input::MouseUp() {
    if (display) {
        XTestFakeButtonEvent(display.get(), Button1, False, 0);
        XFlush(display.get());
    }
}
