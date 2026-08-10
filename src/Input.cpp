#include <chrono>
#include "Input.hpp"

Input::Input() {
}

void Input::PressKey(WORD vk) {
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = vk;
    input[0].ki.dwFlags = 0;
    input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = vk;
	input[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, input, sizeof(INPUT));
}

