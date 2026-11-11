#pragma once

struct _XDisplay; // the X11 display, so this header doesn't need the X11 ones

using Key = unsigned long; // X11 keysym

// Sends fake keyboard and mouse events to the X server

class Input {
public:
    Input();
    ~Input();

    // False if the X display could not be opened. Check it before the rest.
    bool IsReady() const;

    void PressKey(Key keysym);

    // x, y are fractions of the screen (0..1); the button is the left one
    void MoveMouse(float x, float y);
    void MouseDown();
    void MouseUp();

private:
    _XDisplay* display;
};
