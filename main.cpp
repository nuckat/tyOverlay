#define UNICODE
#define _UNICODE
#include <windows.h>
#include <vector>
#include <wchar.h>

#define WSTR(x) x, wcslen(x)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

wchar_t activeWindowTitle[128];

bool highlightI = false, highlightO = false, highlightY = false, highlightM = false, highlightINS = false, highlightM4 = false;
DWORD untilI = 0, untilO = 0, untilY = 0, untilM = 0, untilINS = 0, untilM4 = 0;

void UpdateForegroundWindowTitle() {
    HWND fg = GetForegroundWindow();
    GetWindowTextW(fg, activeWindowTitle, 128);
}


struct MonitorInfo {
    HMONITOR handle;
    RECT rect;
};

std::vector<MonitorInfo> monitors;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT lprcMonitor, LPARAM) {
    monitors.push_back({ hMonitor, *lprcMonitor });
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"MacroOverlayWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30)); // White = transparent area

    RegisterClassW(&wc);

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    int winX = 100;
    int winY = 100;

    if (monitors.size() >= 2) {
        RECT r = monitors[1].rect;
        int overlayWidth = 400; // Match your CreateWindowExW width
        winX = r.right - overlayWidth - 20; // Right-aligned, with 20px padding
        winY = r.top + 20;                  // Top padding stays the same
    }


    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Macro Overlay",
        WS_POPUP,
        winX, winY, 400, 200,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBoxW(NULL, L"CreateWindowExW failed", L"Error", MB_OK);
        return -1;
    }

    SetLayeredWindowAttributes(hwnd, 0, 200, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetTimer(hwnd, 1, 30, NULL);  // ID = 1, every 1000ms
    RegisterHotKey(NULL, 1, 0, VK_F8);


    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY && msg.wParam == 1) {
            PostQuitMessage(0); // Close when ESC pressed
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            SetTextColor(hdc, RGB(0, 255, 0));
            SetBkMode(hdc, TRANSPARENT);
            int leftX = 20;
            int rightX = 200; // adjust depending on width
            int rowY[] = { 20, 50, 80 }; // row positions

            // Left column
            SetTextColor(hdc, highlightM4 ? RGB(255, 255, 255) : RGB(0, 255, 0));
            TextOutW(hdc, leftX, rowY[0],  WSTR(L"[M4] ShadowPlay Record"));

            SetTextColor(hdc, highlightI ? RGB(255, 255, 255) : RGB(0, 255, 0));
            TextOutW(hdc, leftX, rowY[1],  WSTR(L"[ I ] Inventory"));

            SetTextColor(hdc, highlightO ? RGB(255, 255, 255) : RGB(0, 255, 0));
            TextOutW(hdc, leftX, rowY[2],  WSTR(L"[ O ] Objectives"));

            // Right column
            SetTextColor(hdc, highlightINS ? RGB(255, 255, 255) : RGB(0, 255, 0));
            TextOutW(hdc, rightX, rowY[0], WSTR(L"[INS] Steam Overlay"));

            SetTextColor(hdc, highlightY ? RGB(255, 255, 255) : RGB(0, 255, 0));
            TextOutW(hdc, rightX, rowY[1], WSTR(L"[ Y ] Mastery Menu"));

            SetTextColor(hdc, highlightM ? RGB(255, 255, 255) : RGB(0, 255, 0));
            TextOutW(hdc, rightX, rowY[2], WSTR(L"[ M ] Map"));
            // Reset color
            SetTextColor(hdc, RGB(0, 255, 0));
            //Time
            wchar_t timeStr[16];
            SYSTEMTIME st;
            GetLocalTime(&st);
            swprintf(timeStr, 16, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

            // End / Draw Time Live
            TextOutW(hdc, 20, 180, timeStr, wcslen(timeStr));
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_TIMER: {

            // Check Alt+Shift+F10 combo for ShadowPlay highlight
            bool altDown   = (GetAsyncKeyState(VK_MENU)  & 0x8000) != 0;
            bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool f10Down   = (GetAsyncKeyState(VK_F10)   & 0x8000) != 0;

            if (altDown && shiftDown && f10Down) {
                highlightM4 = true;
                untilM4 = GetTickCount() + 500;
            } else if (highlightM4 && GetTickCount() > untilM4) {
                highlightM4 = false;
            }

            auto checkKey = [](int vk, bool& flag, DWORD& until) {
                if (GetAsyncKeyState(vk) & 0x8000) {
                    flag = true;
                    until = GetTickCount() + 300;
                } else if (flag && GetTickCount() > until) {
                    flag = false;
                }
            };

            checkKey('I', highlightI, untilI);
            checkKey('O', highlightO, untilO);
            checkKey('Y', highlightY, untilY);
            checkKey('M', highlightM, untilM);
            checkKey(VK_INSERT, highlightINS, untilINS);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }


        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}