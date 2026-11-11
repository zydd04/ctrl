#pragma once
#include <memory>

struct _XDisplay;

using Key = unsigned long; // X11 keysym

struct DisplayCloser {
    void operator()(_XDisplay* d) const;
};

// Sends fake keyboard and mouse events to the X server

class Input {
public:
    Input();

    bool IsReady() const;

    void PressKey(Key keysym);

    // x, y are fractions of the screen (0..1); the button is the left one
    void MoveMouse(float x, float y);
    void MouseDown();
    void MouseUp();

private:
    std::unique_ptr<_XDisplay, DisplayCloser> display;
};
