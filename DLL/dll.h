#pragma once
#ifdef _WINDLL
	#define DLL_API __declspec(dllexport)
#else
	#define DLL_API __declspec(dllimport)
#endif

DLL_API int InitializeSwag();