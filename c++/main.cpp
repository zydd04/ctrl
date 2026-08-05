#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM); //forward declaratio
void AddMenu(HWND);
HMENU hMenu; //menu handler
HMENU hDropMenu;
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
    CreateWindowW(L"myWinClass", L"CTRLcv", WS_OVERLAPPEDWINDOW | WS_VISIBLE,100,100,500,500,NULL,NULL,NULL,NULL);
    
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
        case 1:
            MessageBeep(MB_OK); //click sound
            return 0;
        
        default:
            break;
        }
    case WM_CREATE:
        AddMenu(hWnd); //create menu when window is created and get hwnd
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);//quit when x clicked 
        return 0;
    
    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }

}

void AddMenu(HWND hWnd) {
    hMenu = CreateMenu();
    hDropMenu = CreateMenu(); //Drop dowm menu

    //menu components
    AppendMenuW(hMenu,MF_STRING,1,L"Connect");
    AppendMenuW(hMenu,MF_POPUP,(UINT_PTR)hDropMenu,L"Help");
    AppendMenuW(hMenu,MF_STRING,2,L"About");
    //Drop menu components
    AppendMenuW(hDropMenu, MF_STRING,3, L"How it Works?");
    AppendMenuW(hDropMenu, MF_STRING, 4, L"FAQ");
    //set menu to window
    SetMenu(hWnd, hMenu);
}