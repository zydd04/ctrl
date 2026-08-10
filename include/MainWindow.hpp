#pragma once
#include <Windows.h>
#include <string>
#include "Capture.hpp"
#include "Detector.hpp"
#include "Input.hpp"

//Creates the main window of the app
class MainWindow {
public:
	explicit MainWindow(int wWidth = 800, int wHeight = 400);
	
	//Returns hwnd handle
	HWND getH() const {return hwnd_;}

	//Handles Errors
	void ShowError(std::wstring& text, std::wsting& title) const;
	//Shows information
	void ShowInfo(std::wstring& text, std::wstring& title) const;

	//Starts the Webcam for testing of functionality of the app
	void StartStreamTest() const;
	
	//Runs the App
	void Run() const;

	//Stops the App
	void Stop() const;
private:
	LRESULT CALLBACK WindowProcess(HWND, UINT, WPARAM, LPARAM);
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

	int wWidth_;
	int wHeight_;

	void AddMenu();
	void AddControl();

	HWND hwnd_ = nullptr;
	HMENU menu_ = nullptr;
	
	enum MenuId = {Connectid = 1, Helpid = 2, Aboutid = 4, Stopid = 5, Runid = 6, Testid = 7}
};	



