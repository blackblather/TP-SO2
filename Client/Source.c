#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "dll.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HBRUSH hbrBkgnd = NULL;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	//Source: https://docs.microsoft.com/en-us/windows/desktop/learnwin32/your-first-windows-program
	// Register the window class.
	const TCHAR CLASS_NAME[] = TEXT("Sample Window Class");

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = COLOR_WINDOW + 1;
	//wc.hInstance = LoadInstance
	//wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCEA(IDI_ICON1));
	//wc.lpszMenuName = MAKEINTRESOURCEA(IDR_MENU1);

	RegisterClass(&wc);

	// Create the window.

	HWND hwnd = CreateWindowEx(
		0,								// Optional window styles.
		CLASS_NAME,						// Window class
		TEXT("Arkanoid Client v1.0"),   // Window text
		WS_OVERLAPPEDWINDOW,			// Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 170,

		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL)
		return 0;

	//Show window
	ShowWindow(hwnd, nCmdShow);

	//Init message loop
	MSG msg;
	memset(&msg, 0, sizeof(MSG));
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_CREATE: {
			//Create label
			CreateWindow(TEXT("Static"),
				TEXT("Enter username"),
				WS_CHILD | WS_VISIBLE,
				50,
				20,
				150,
				25,
				hwnd,
				NULL,
				NULL,
				NULL);

			//Create textbox
			HWND usernameTxt = CreateWindow(TEXT("Edit"),
				TEXT(""),
				WS_BORDER | WS_CHILD | WS_VISIBLE,
				50,
				50,
				200,
				25,
				hwnd,
				NULL,
				NULL,
				NULL);

			return 0;
		} break;
		case WM_CTLCOLORSTATIC: {
			//StackOverflow: https://stackoverflow.com/questions/4495509/static-control-background-color-with-c
			//MSDN: https://docs.microsoft.com/en-us/windows/desktop/controls/wm-ctlcolorstatic
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkColor(hdcStatic, RGB(255, 255, 255));

			if (hbrBkgnd == NULL)
			{
				hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
			}
			return (INT_PTR)hbrBkgnd;
		}
		case WM_CLOSE: {
			if (MessageBox(hwnd, TEXT("Really quit?"), TEXT("My application"), MB_OKCANCEL) == IDOK)
				DestroyWindow(hwnd);
			return 0;
		} break;
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		} break;
		default: {
			//Performs default action for unhandled messages
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		} break;
	}
}