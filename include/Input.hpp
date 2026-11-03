#pragma once
#include <memory>

struct _XDisplay;

using Key = unsigned long;

struct DisplayCloser {
    void operator()(_XDisplay* d) const;
};

class Input {
public:
    Input();

    bool IsReady() const;
    void PressKey(Key keysym);

private:
    std::unique_ptr<_XDisplay, DisplayCloser> display;
};