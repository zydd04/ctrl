#pragma once

struct _XDisplay; // X11 display

using Key = unsigned long; // X11 keysym

// Fake keyboard and mouse events

class Input {
public:
    Input();
    ~Input();

    bool IsReady() const;
    void PressKey(Key keysym);

    // x, y from 0 to 1 of the screen
    void MoveMouse(float x, float y);
    void MouseDown();
    void MouseUp();

private:
    _XDisplay* display;
};
