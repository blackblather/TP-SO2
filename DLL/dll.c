#include "dll.h"

//"private" vars
//(FILE VIEWS)
HANDLE hFileMapping = NULL;
_gameData* gameDataStart = NULL;
LPVOID messageStart = NULL;
SYSTEM_INFO sysInfo;
//(NEW USERS)
HANDLE hNewUserMutex = NULL;
HANDLE hNewUserFinishedWritingEvent = NULL;
//------------------------------------------------------

//"private" functions
//(MAPPED FILE)
BOOL OpenExistingGameMappedFile() {
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
LPVOID LoadFileView(int offset, int size) {
	return MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ | FILE_MAP_WRITE,
		0,
		offset,
		size);
} 
BOOL LoadGameMappedFileResources() {
	return (OpenExistingGameMappedFile() &&
		(gameDataStart = (_gameData*)LoadFileView(0, sysInfo.dwAllocationGranularity)) != NULL &&
		(messageStart = LoadFileView(sysInfo.dwAllocationGranularity, sysInfo.dwAllocationGranularity)) != NULL);
}
//(NEW USER)
BOOL LoadNewUserResources() {
	hNewUserMutex = OpenMutex(SYNCHRONIZE, //Only the SYNCHRONIZE access right is required to use a mutex
		FALSE, //Child processess do NOT inherit this mutex
		_T("newUserMutex") //Mutex name
	);
	if (hNewUserMutex != NULL) { //Opened mutex successfuly
		hNewUserFinishedWritingEvent = OpenEvent(SYNCHRONIZE, //The right to use the object for synchronization
			FALSE, //Child processess do NOT inherit this mutex
			_T("newUserFinishedWritingEvent") //Mutex name
		);
		if (hNewUserFinishedWritingEvent != NULL)	//Opened event successfuly
			return TRUE;
	}
	return FALSE;
}
void SendUsername(char username[256]) {
	//Write to mapped file and trigger event
	//
}
void ReadLoginResponse(BOOL* resp) {
	//wait for event, and get server response
}
//------------------------------------------------------

//"public" functions
BOOL LoadGameResources() {
	return (LoadGameMappedFileResources() &&
			LoadNewUserResources());
}
BOOL LoggedIn(char username[256]) {
	BOOL resp = FALSE;
	WaitForSingleObject(hNewUserMutex, INFINITE);
	SendUsername(username);
	ReadLoginResponse(&resp);	
	ReleaseMutex(hNewUserMutex);
	return resp;
}