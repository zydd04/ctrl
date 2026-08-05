#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {

    WNDCLASSW wc = {0};
    wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = L"myWinClass";
    wc.lpfnWndProc = WindowProc;
    
    if (!RegisterClassW(&wc)) {
        return -1;
    }

    CreateWindowW(L"myWinClass", L"CTRLcv", WS_OVERLAPPEDWINDOW | WS_VISIBLE,100,100,500,500,NULL,NULL,NULL,NULL);
    
    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

    switch ( msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    
    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }

}