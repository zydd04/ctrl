#pragma once
#include <memory>
#include <string>

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

    // Human readable name of a key, e.g. "space", "Right", "a"
    static std::string KeyName(Key keysym);

private:
    std::unique_ptr<_XDisplay, DisplayCloser> display;
};