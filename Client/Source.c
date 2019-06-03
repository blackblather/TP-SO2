#include "dll.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HBRUSH hbrBkgnd = NULL;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	if (!LoadGameResources())
		MessageBox(NULL, TEXT("Error loading game resources.\nExiting..."), TEXT("Fatal error"), MB_OK | MB_ICONERROR);
	else {
		//Source: https://docs.microsoft.com/en-us/windows/desktop/learnwin32/your-first-windows-program
		// Register the window class.
		const TCHAR CLASS_NAME[] = TEXT("Sample Window Class");

		WNDCLASS wc;
		memset(&wc, 0, sizeof(WNDCLASS));
		HCURSOR cursor;

		wc.lpfnWndProc = WindowProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = CLASS_NAME;
		wc.hbrBackground = COLOR_WINDOW + 1;
		//Source 1: https://docs.microsoft.com/en-us/windows/desktop/menurc/about-cursors
		//Source 2: https://docs.microsoft.com/en-us/windows/desktop/api/Winuser/nf-winuser-loadcursora
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
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
	}
	return 0;
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//Static vars: https://www.geeksforgeeks.org/static-variables-in-c/
	static HWND hUsernameTxt = NULL;	//static vars in C/C++ preserve their value between function calls
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
			hUsernameTxt = CreateWindow(TEXT("Edit"),
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

			//Create "Play" button
			CreateWindow(
				TEXT("Button"),								// Predefined class; Unicode assumed 
				TEXT("Play"),								// Button text 
				WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,	// Styles 
				50,											// x position 
				85,											// y position 
				80,											// Button width
				25,											// Button height
				hwnd,										// Parent window
				(HMENU)1,									// child-window identifier
				NULL,										// hInstance not needed
				NULL);										// Pointer not needed
			
			return 0;
		} break;
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case 1: {
					TCHAR username[256];
					GetWindowText(hUsernameTxt, username, 256);
					/* Esta função (LoggedIn(...)) é bloqueante e não deveria ser chamada na thread que trata da GUI,
					 * mas parto do principio que o servidor não levará muito tempo a responder,
					 * pelo que não creio que seja necessário iniciar uma nova thread apenas para chamar a função.*/
					if(LoggedIn(username))
						MessageBox(NULL, TEXT("Great success"), TEXT("Great success"), MB_OK | MB_ICONINFORMATION);
					else
						MessageBox(NULL, TEXT("Great shit"), TEXT("Great shit"), MB_OK | MB_ICONERROR);
				} break;
			}
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