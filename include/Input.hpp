#pragma once
#include <Windows.h>
#include <unordered_map>
#include <chrono>

//Sends virtual key presses to the OS
 
class Input {
public:
	explicit Input(); 

	//Press and release key 
	void PressKey(WORD vk);
};
