#pragma once
#include <windows.h>
#include <tchar.h>
#include "Resources.h"


#ifdef _WINDLL
	#define DLL_API __declspec(dllexport)
#else
	#define DLL_API __declspec(dllimport)
#endif

//Vars to export
DLL_API INT clientAreaWidth, clientAreaHeight;

//Functions to export
DLL_API VOID CloseSharedInfoHandles();
DLL_API BOOL LoadGameResources();
DLL_API BOOL LoggedIn(TCHAR username[USERNAME_MAX_LENGHT]);
DLL_API void WritePlayerMsg(WPARAM wParam);
DLL_API BOOL InitUpdateBaseThread(HWND* hGameWnd);
DLL_API void PrintGameData(HDC hdc, HWND hwnd);