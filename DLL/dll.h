#pragma once
#include <windows.h>
#include <tchar.h>
//$(SolutionDir)Server
#include "C:\\Users\\joaom\\Google Drive\\Licenciatura (2016-xxxx)\\2018 - 2019\\Sem2\\SO2\\TP\\Server\\Resources.h"

#ifdef _WINDLL
	#define DLL_API __declspec(dllexport)
#else
	#define DLL_API __declspec(dllimport)
#endif

//Functions
DLL_API BOOL LoadGameResources();
DLL_API void SendUsername(char username[256]);
DLL_API BOOL LoggedIn(char username[256]);