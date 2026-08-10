#include "Input.hpp"

int main() {
	Input input;
	for (int i = 0; i < 4; ++i) {
		input.PressKey(VK_SPACE);
		Sleep(50);
		input.PressKey('A');
		Sleep(50);
		input.PressKey('D');
		Sleep(50);
	}
	return 0;
}
