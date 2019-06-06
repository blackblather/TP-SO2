#include "dll.h"

//"private" vars
//(FILE VIEWS)
HANDLE hFileMapping = NULL;
LPVOID messageBaseAddr;
_gameData* gameDataStart = NULL;
_gameMsgNewUser* gameMsgNewUser = NULL;
_serverResponse* serverResp;
_clientMsg* messageStart;
SYSTEM_INFO sysInfo;
//(NEW USERS)
HANDLE hNewUserMutex = NULL;
HANDLE hNewUserServerEvent = NULL;
HANDLE hNewUserClientEvent = NULL;
//(PLAYER MSG)
HANDLE hNewPlayerMsgMutex = NULL;
HANDLE hNewPlayerMsgSemaphore = NULL;
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
	if (OpenExistingGameMappedFile() &&
		(gameDataStart = (_gameData*)LoadFileView(0, sysInfo.dwAllocationGranularity)) != NULL &&
		(messageBaseAddr = LoadFileView(sysInfo.dwAllocationGranularity, sysInfo.dwAllocationGranularity)) != NULL) {
		messageStart = (_clientMsg*)messageBaseAddr;
		gameMsgNewUser = (_gameMsgNewUser*)messageStart;
		messageStart = (_gameMsgNewUser*)messageStart + 1;
		serverResp = (_serverResponse*)messageStart;
		messageStart = (_serverResponse*)messageStart + 1;
		return TRUE;
	}
	return FALSE;
}
//(NEW USER)
BOOL LoadNewUserResources() {
	hNewUserMutex = OpenMutex(SYNCHRONIZE, //Only the SYNCHRONIZE access right is required to use a mutex
		FALSE, //Child processess do NOT inherit this mutex
		_T("newUserMutex") //Mutex name
	);
	if (hNewUserMutex != NULL) { //Opened mutex successfuly
		hNewUserServerEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, //The right to use the object for synchronization
			FALSE, //Child processess do NOT inherit this mutex
			_T("newUserServerEvent") //Event name
		);
		if (hNewUserServerEvent != NULL) {	//Opened event successfuly
			hNewUserClientEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, //The right to use the object for synchronization
				FALSE, //Child processess do NOT inherit this mutex
				_T("newUserClientEvent") //Event name
			);
			if (hNewUserClientEvent != NULL)	//Opened event successfuly
				return TRUE;
		}
	}
	return FALSE;
}
void SendUsername(TCHAR username[USERNAME_MAX_LENGHT]) {
	//Write to mapped file and trigger event
	_tcscpy_s(gameMsgNewUser->username, USERNAME_MAX_LENGHT, username);
	SetEvent(hNewUserServerEvent);
}
void ReadLoginResponse(BOOL* response) {
	//wait for event, and get server response
	WaitForSingleObject(hNewUserClientEvent, INFINITE);
	*response = gameMsgNewUser->response;
	return;
}
//(PLAYER MSG)
BOOL LoadPlayerMsgResources() {
	hNewPlayerMsgMutex = OpenMutex(SYNCHRONIZE, //Only the SYNCHRONIZE access right is required to use a mutex
		FALSE, //Child processess do NOT inherit this mutex
		_T("newPlayerMsgMutex") //Mutex name
	);
	if (hNewPlayerMsgMutex != NULL) {
		hNewPlayerMsgSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
			FALSE, //Child processess do NOT inherit this semaphore
			TEXT("newPlayerMsgSemaphore"));	//Semaphore name
		if (hNewPlayerMsgSemaphore != NULL)
			return TRUE;
	}
	return FALSE;
}
BOOL MsgIteratorReachedTheEnd() {
	return TRUE;
}
//------------------------------------------------------

//"public" functions
VOID CloseSharedInfoHandles() {
	UnmapViewOfFile(messageBaseAddr);
	UnmapViewOfFile((LPVOID)gameDataStart);
	CloseHandle(hFileMapping);
}
BOOL LoadGameResources() {
	return (LoadGameMappedFileResources() &&
			LoadNewUserResources() &&
			LoadPlayerMsgResources());
}
BOOL LoggedIn(TCHAR username[USERNAME_MAX_LENGHT]) {
	BOOL response = FALSE;
	WaitForSingleObject(hNewUserMutex, INFINITE);
	SendUsername(username);
	ReadLoginResponse(&response);
	ReleaseMutex(hNewUserMutex);
	return response;
}
void WritePlayerMsg(WPARAM wParam) {

}