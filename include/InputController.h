#pragma once
#include <Windows.h>
#include <unordered_map>
#include <chrono>

//Sends virtual key presses to the OS
//Prevents spam using a cooldown using a hashmap that stores last time a key was pressed
 
class InputController {
public:
	explicit InputController(std::chrono::milliseconds cooldown = std::chrono::milliseconds(300)); 

	//Press and release key 
	void PressKey(WORD vk) const;

private:
	std::chrono::milliseconds cooldown_;
	std::chrono::milliseconds lastPress;
};
