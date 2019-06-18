#include "dll.h"

//"private" vars
//(FILE VIEWS)
HANDLE hFileMapping = NULL;
LPVOID messageBaseAddr;
_gameData* gameDataStart = NULL;
_gameMsgNewUser* gameMsgNewUser = NULL;
_clientMsg* clientMsg;
SYSTEM_INFO sysInfo;
//(NEW USERS)
HANDLE hNewUserMutex = NULL;
HANDLE hNewUserServerEvent = NULL;
HANDLE hNewUserClientEvent = NULL;
TCHAR updateMapEventName[20];	//Used in (UPDATE MAP) but recieved in (NEW USERS)
//(PLAYER MSG)
HANDLE hNewPlayerMsgMutex = NULL;
HANDLE hNewPlayerMsgSemaphore = NULL;
INT clientId = -1;
//(UPDATE MAP)
HANDLE hReadUpdatedBaseThread = NULL;
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
		gameMsgNewUser = (_gameMsgNewUser*)messageBaseAddr;
		clientMsg = gameMsgNewUser + 1;
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
void ReadLoginResponse(BOOL* loggedIn) {
	//wait for event, and get server response
	WaitForSingleObject(hNewUserClientEvent, INFINITE);
	if ((*loggedIn = gameMsgNewUser->loggedIn) == TRUE) {
		clientId = gameMsgNewUser->clientId;
		_tcscpy_s(updateMapEventName, 20, gameMsgNewUser->updateMapEventName);
		clientAreaWidth = gameMsgNewUser->clientAreaWidth;
		clientAreaHeight = gameMsgNewUser->clientAreaHeight;
	}
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
BOOL ClientMsgPosReachedTheEnd(INT pos, INT max) {
	return (pos == (max - 1));
}
BOOL IsValidMove(WPARAM wParam) {
	return (wParam == VK_LEFT || wParam == VK_RIGHT);
}
BOOL SlotIsFree(INT currentPos) {
	return clientMsg[currentPos].move == none;
}
//(UPDATE MAP)
DWORD WINAPI UpdateMapThread(LPVOID lpParameter) {
	HWND* hGameWnd = (HWND*)lpParameter;
	HANDLE hUpdateMapEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, //The right to use the object for synchronization
		FALSE, //Child processess do NOT inherit this mutex
		updateMapEventName //Event name
	);
	if (hUpdateMapEvent != NULL) {
		while (1) {
			WaitForSingleObject(hUpdateMapEvent, INFINITE);
			InvalidateRect(*hGameWnd, NULL, TRUE);
			UpdateWindow(*hGameWnd);
		}
	}
	
	return 0;
}
//------------------------------------------------------

//"public" functions
//(MAPPED FILE)
VOID CloseSharedInfoHandles() {
	UnmapViewOfFile(messageBaseAddr);
	UnmapViewOfFile((LPVOID)gameDataStart);
	CloseHandle(hFileMapping);
}
//(NEW USER)
BOOL LoggedIn(TCHAR username[USERNAME_MAX_LENGHT]) {
	BOOL response = FALSE;
	WaitForSingleObject(hNewUserMutex, INFINITE);
	SendUsername(username);
	ReadLoginResponse(&response);
	ReleaseMutex(hNewUserMutex);
	return response;
}
//(PLAYER MSG)
void WritePlayerMsg(WPARAM wParam) {
	WaitForSingleObject(hNewPlayerMsgMutex, INFINITE);
	if (ClientMsgPosReachedTheEnd(gameDataStart->clientMsgPos, gameDataStart->maxClientMsgs))
		gameDataStart->clientMsgPos = 0;
	if (IsValidMove(wParam) && SlotIsFree(gameDataStart->clientMsgPos)) {
		clientMsg[gameDataStart->clientMsgPos].clientId = clientId;
		clientMsg[gameDataStart->clientMsgPos].move = (wParam == VK_LEFT ? moveLeft : moveRight);
		gameDataStart->clientMsgPos++;
		ReleaseSemaphore(hNewPlayerMsgSemaphore, 1, NULL);
	}
	ReleaseMutex(hNewPlayerMsgMutex);
}
//(UPDATE MAP)
BOOL InitUpdateBaseThread(HWND* hGameWnd) {
	hReadUpdatedBaseThread = CreateThread(
		NULL,	//hThreadNewUsers cannot be inherited by child processes
		0,	//Default stack size
		UpdateMapThread,	//Function to execute
		(LPVOID)hGameWnd,	//Function param
		0,	//The thread runs immediately after creation
		NULL	//Thread ID is not stored anywhere
	);
	return hReadUpdatedBaseThread != NULL;
}
void PrintGameData(HDC hdc, HWND hwnd) {
	//Source: https://docs.microsoft.com/en-us/previous-versions/ms969905(v=msdn.10)
	RECT clientArea;
	HDC hdcMem, hbmOld;
	HBITMAP hbmMem;
	HBRUSH hBrushBaseIn = CreateSolidBrush(RGB(114, 116, 128));
	HBRUSH hBrushBallIn = CreateSolidBrush(RGB(0, 116, 128));
	HBRUSH hBrushIn = CreateSolidBrush(RGB(255, 0, 0));
	HBRUSH hBrushOut = CreateSolidBrush(RGB(0, 0, 0));

	// Get the size of the client rectangle.
	GetClientRect(hwnd, &clientArea);

	// Create a compatible DC.
	hdcMem = CreateCompatibleDC(hdc);
	//Set logical coords to MM_TEXT
	SetMapMode(hdcMem, MM_TEXT);
	// Create a bitmap big enough for our client rectangle.
	hbmMem = CreateCompatibleBitmap(hdc,
			clientArea.right - clientArea.left,
			clientArea.bottom - clientArea.top);

	// Select the bitmap into the off-screen DC.
	hbmOld = SelectObject(hdcMem, hbmMem);

	// Erase the background.
	HBRUSH hbrBkGnd = CreateSolidBrush(GetDCBrushColor(hdc));
	FillRect(hdcMem, &clientArea, hbrBkGnd);
	DeleteObject(hbrBkGnd);

	
	for (int i = 0; i < MAX_BLOCKS; i++) {
		FillRect(hdcMem, &gameDataStart->block[i].rectangle, hBrushIn);
		FrameRect(hdcMem, &gameDataStart->block[i].rectangle, hBrushOut);
	}
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (gameDataStart->base[i].hasPlayer) {
			FillRect(hdcMem, &gameDataStart->base[i].rectangle, hBrushBaseIn);
			FrameRect(hdcMem, &gameDataStart->base[i].rectangle, hBrushOut);
		}
	}
	for (int i = 0; i < MAX_BALLS; i++) {
		if (gameDataStart->ball[i].isActive) {
			FillRect(hdcMem, &gameDataStart->ball[i].rectangle, hBrushBallIn);
			FrameRect(hdcMem, &gameDataStart->ball[i].rectangle, hBrushOut);
		}
	}

	BitBlt(hdc,
		clientArea.left,
		clientArea.top,
		clientArea.right - clientArea.left,
		clientArea.bottom - clientArea.top,
		hdcMem,
		0, 0,
		SRCCOPY);

	SelectObject(hdcMem, hbmOld);
	DeleteObject(hbmMem);
	DeleteObject(hbmOld);
	DeleteObject(hBrushBaseIn);
	DeleteObject(hBrushBallIn);
	DeleteObject(hBrushIn);
	DeleteObject(hBrushOut);
	DeleteDC(hdcMem);
}
//(SERVER INITIALIZATION)
BOOL LoadGameResources() {
	return (LoadGameMappedFileResources() &&
		LoadNewUserResources() &&
		LoadPlayerMsgResources());
}