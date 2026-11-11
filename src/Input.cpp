#include "Input.hpp"

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

Input::Input() : display(XOpenDisplay(nullptr)) {}

Input::~Input() {
    if (display) {
        XCloseDisplay(display);
    }
}

bool Input::IsReady() const {
    return display != nullptr;
}

void Input::PressKey(Key keysym) {
    KeyCode keycode = XKeysymToKeycode(display, keysym);
    XTestFakeKeyEvent(display, keycode, True, 0);
    XTestFakeKeyEvent(display, keycode, False, 0);
    XFlush(display);
}

void Input::MoveMouse(float x, float y) {
    int screen = DefaultScreen(display);
    int px = x * (DisplayWidth(display, screen) - 1);
    int py = y * (DisplayHeight(display, screen) - 1);
    XTestFakeMotionEvent(display, screen, px, py, 0);
    XFlush(display);
}

void Input::MouseDown() {
    XTestFakeButtonEvent(display, Button1, True, 0);
    XFlush(display);
}

void Input::MouseUp() {
    XTestFakeButtonEvent(display, Button1, False, 0);
    XFlush(display);
}
