#include <Windows.h>

const int CONNECT_MENU = 1;
const int HELP_MENU_HOW = 2;
const int ABOUT_MENU = 4;
const int STOP_MENU = 5;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM); //forward declaratio
void AddMenu(HWND);

HMENU hMenu; //menu handler
HMENU hDropMenu;
HWND hCtrl1 = NULL;
HWND hCtrl2 = NULL;
HWND hCtrl3 = NULL;

void AddControl(HWND);

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
            //show original screen
            AddControl(hWnd);
            return 0;
        case STOP_MENU:
            DestroyWindow(hWnd);
            return 0;
        case HELP_MENU_HOW:
            MessageBoxW(hWnd, 
                L"1. Select your webcam hardware.\n2. Click 'Connect' in the menu.\n3. Click 'Run' to start streaming.", 
                L"How it Works", MB_ICONINFORMATION);//message boc 
            return 0;
        case ABOUT_MENU:
            MessageBoxW(hWnd,L"This Was Developed as fun learning peoject.\nGithub: @Zydd04",L"About",MB_ICONINFORMATION);
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
    hDropMenu = CreateMenu(); //Drop dowm menu

    //menu components
    AppendMenuW(hMenu,MF_STRING,CONNECT_MENU,L"Connect");
    AppendMenuW(hMenu, MF_STRING, STOP_MENU, L"Stop");
    AppendMenuW(hMenu,MF_POPUP,(UINT_PTR)hDropMenu,L"Help");
    AppendMenuW(hMenu,MF_STRING,ABOUT_MENU,L"About");
    //Drop menu components
    AppendMenuW(hDropMenu, MF_STRING,HELP_MENU_HOW, L"How it Works?");
    //set menu to window
    SetMenu(hWnd, hMenu);
}

void AddControl(HWND hWnd) {
    hCtrl1 = CreateWindowW(L"Static", L"Please Connect Your WebCam", WS_VISIBLE | WS_CHILD, 300, 50, 300, 50, hWnd,0,0,0);//text
    hCtrl2 = CreateWindowW(L"Static", L"Select WebCam:", WS_VISIBLE | WS_CHILD, 350, 100, 300, 50, hWnd,0,0,0);//text static
    hCtrl3 = CreateWindowW(L"Button", L"Run", WS_VISIBLE | WS_CHILD, 370, 250, 40, 30, hWnd,0,0,0);
}

void ClearWindow() {
    if (hCtrl1) {
        DestroyWindow(hCtrl1);
        hCtrl1 = NULL;
    }//Destroy text1
    if (hCtrl2) {
        DestroyWindow(hCtrl2);
        hCtrl2 = NULL;
    }//Remove text 2
    if (hCtrl3) {
        DestroyWindow(hCtrl3);
        hCtrl3 = NULL;
    }//remove button
}