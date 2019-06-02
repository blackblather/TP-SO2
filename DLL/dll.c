#include "dll.h"

//"private" vars
HANDLE hFileMapping = NULL;
_gameData* gameDataStart = NULL;
LPVOID messageStart = NULL;
SYSTEM_INFO sysInfo;

//"private" functions
BOOL OpenExistingGameMappedFile() {
	if (hFileMapping == NULL) {
		GetSystemInfo(&sysInfo);
		hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,										//Size in bytes	(high-order)
			sysInfo.dwAllocationGranularity * 2,	//Size in bytes (low-order)
			_T("LocalSharedInfo"));
		if (hFileMapping != NULL) {
			if (GetLastError() == ERROR_ALREADY_EXISTS)
				return TRUE;	//Return handle to file mapped by the server
			//If the code reaches here, that means the file was NOT mapped in memory
			//(a.k.a. the server's not running), so close the handle and set hFileMapping to NULL
			CloseHandle(hFileMapping);
			hFileMapping = NULL;
		}
		return FALSE;
	}
	return TRUE;
}
LPVOID LoadFileView(int offset, int size) {
	return MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ | FILE_MAP_WRITE,
		0,
		offset,
		size);
} 

//"public" functions
BOOL LoadGameFileViews() {
	return (OpenExistingGameMappedFile() &&
		(gameDataStart = (_gameData*)LoadFileView(0, sysInfo.dwAllocationGranularity)) != NULL &&
		(messageStart = LoadFileView(sysInfo.dwAllocationGranularity, sysInfo.dwAllocationGranularity)) != NULL);
}

void SendUsername(char username[256]) {

}

BOOL LoggedIn(char username[256]) {
	SendUsername(username);
}