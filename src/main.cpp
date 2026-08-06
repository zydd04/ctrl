#include <Windows.h>

const int CONNECT_MENU = 1;
const int HELP_MENU = 2;
const int ABOUT_MENU = 4;
const int STOP_MENU = 5;
const int RUN = 6;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM); //forward declaratio
void AddMenu(HWND);

HMENU hMenu; //menu handler


void AddControl(HWND);
void ConnectWebcam();
bool isConn = false;
void StartStream();
bool isStrm = false;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {

    WNDCLASSW wc = {0}; //aloc memory

    wc.hbrBackground = (HBRUSH) COLOR_WINDOW; //default background color
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); //std arrow mouse
    wc.hInstance = hInst; //instance of runing app
    wc.lpszClassName = L"myWinClass";//unique text name
    wc.lpfnWndProc = WindowProc;//event handler function
    
    if (!RegisterClassW(&wc)) { // register with os, if fail quit
        return -1;
    }

    //create window
    CreateWindowW(L"myWinClass", L"CTRLcv", WS_OVERLAPPEDWINDOW | WS_VISIBLE,100,100,800,400,NULL,NULL,NULL,NULL);
    
    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); //raw keyboard signals to readble chars
        DispatchMessage(&msg); //forward to windowProc
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

    switch (msg) // look at msg ID
    {
    case WM_COMMAND: //user clicks handling -> WPARAM wp
        switch (wp)
        {
        case CONNECT_MENU:
            ConnectWebcam();
            if (!isConn) {
                MessageBoxW(hWnd, L"Failed to connect webcam", L"Error", MB_OK | MB_ICONERROR);
            }
            else {
                MessageBoxW(hWnd, L"Webcam Connected", L"Success", MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        case RUN:
            if (!isConn) {
                MessageBoxW(hWnd, L"WebCam not Connected. Please Connect First", L"Error", MB_OK | MB_ICONERROR);
            }
            else {
                StartStream();
                MessageBoxW(hWnd, L"Streaming on", L"Success", MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        case STOP_MENU:
            DestroyWindow(hWnd);
            return 0;
        case HELP_MENU:
            MessageBoxW(hWnd, 
                L"1. You need a webcam for this.\n2. Click 'Connect' in the menu.\n3. Click 'Run' to start streaming and run the program.", 
                L"How it Works", MB_ICONINFORMATION);//message boc 
            return 0;
        case ABOUT_MENU:
            MessageBoxW(hWnd,L"This Was Developed as fun learning peoject.\n                Github: @Zydd04",L"About",MB_ICONINFORMATION);
            return 0;
        default:
            break;
        }
    case WM_CREATE:
        AddMenu(hWnd); //create menu when window is created and get hwnd
        AddControl(hWnd); //add hwnd to func needed parent window
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);//quit when x clicked 
        return 0;
    
    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

void AddMenu(HWND hWnd) {
    hMenu = CreateMenu();

    //menu components
    AppendMenuW(hMenu,MF_STRING,CONNECT_MENU,L"Connect");
    AppendMenuW(hMenu, MF_STRING, STOP_MENU, L"Stop");
    AppendMenuW(hMenu,MF_POPUP,HELP_MENU,L"Help");
    AppendMenuW(hMenu,MF_STRING,ABOUT_MENU,L"About");
    //set menu to window
    SetMenu(hWnd, hMenu);
}

void AddControl(HWND hWnd) {
    CreateWindowW(L"Static", L"Please Connect Your WebCam", WS_VISIBLE | WS_CHILD, 300, 50, 300, 50, hWnd,0,0,0);//text
    CreateWindowW(L"Button", L"Run", WS_VISIBLE | WS_CHILD, 370, 150, 40, 30, hWnd,(HMENU)RUN,0,0);
}

//check webcam connection
void ConnectWebcam() {
    isConn = true;
}
void StartStream() {
    isStrm = true;
}
