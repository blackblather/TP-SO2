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
TCHAR updateBaseEventName[21];	//Used in (UPDATE MAP) but recieved in (NEW USERS)
//(PLAYER MSG)
HANDLE hNewPlayerMsgMutex = NULL;
HANDLE hNewPlayerMsgSemaphore = NULL;
INT clientId = -1;
//(UPDATE MAP)
HANDLE hReadUpdatedBaseThread = NULL;
_gameData cachedGameData;
RECT deletedBasePart, newRectArea;
BOOL firstPaint = TRUE;
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
		_tcscpy_s(updateBaseEventName, 21, gameMsgNewUser->updateBaseEventName);
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
BOOL BlockWasDestroyed(_block oldBlock, _block newBlock) {
	//Blocks always go from "Non destroyed" to "Destroyed"
	//Never the other way around
	return oldBlock.destroyed != newBlock.destroyed;
}
BOOL BaseMoved(_base oldBase, _base newBase) {
	//Bases only move sideways
	return oldBase.rectangle.left != newBase.rectangle.left;
}
BOOL BallMoved(_ball oldBall, _ball newBall) {
	//Balls move on both axis
	return (oldBall.rectangle.left != newBall.rectangle.left ||
			oldBall.rectangle.top != newBall.rectangle.top);
}
void CompareAndInvalidateBases(HWND hGameWnd) {
	//for (int i = 0; i < MAX_BLOCKS; i++)
		//if (BlockWasDestroyed(cachedGameData.block[i], gameDataStart->block[i])) 
			//InvalidateRect(hGameWnd, &cachedGameData.block[i].rectangle, TRUE);
	for (int i = 0; i < MAX_PLAYERS; i++)
		if (BaseMoved(cachedGameData.base[i], gameDataStart->base[i])) {
			SubtractRect(&deletedBasePart, &cachedGameData.base[i].rectangle, &gameDataStart->base[i]);
			UnionRect(&newRectArea, &deletedBasePart, &gameDataStart->base[i].rectangle);
			InvalidateRect(hGameWnd, &newRectArea, FALSE);
		}
	//for (int i = 0; i < MAX_BALLS; i++)
		//if (BallMoved(cachedGameData.ball[i], gameDataStart->ball[i]))
			//InvalidateRect(hGameWnd, &cachedGameData.ball[i].rectangle, TRUE);
}
void CopyGameData() {
	for (int i = 0; i < MAX_BLOCKS; i++)
		cachedGameData.block[i] = gameDataStart->block[i];
	for (int i = 0; i < MAX_PLAYERS; i++)
		cachedGameData.base[i] = gameDataStart->base[i];
	for (int i = 0; i < MAX_BALLS; i++)
		cachedGameData.ball[i] = gameDataStart->ball[i];
	cachedGameData.clientMsgPos = gameDataStart->clientMsgPos;
	cachedGameData.maxClientMsgs = gameDataStart->maxClientMsgs;
}
DWORD WINAPI UpdateBasesThread(LPVOID lpParameter) {
	HWND* hGameWnd = (HWND*)lpParameter;
	HANDLE hUpdateBaseEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, //The right to use the object for synchronization
		FALSE, //Child processess do NOT inherit this mutex
		updateBaseEventName //Event name
	);
	HANDLE hBaseConfirmationSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
		FALSE, //Child processess do NOT inherit this semaphore
		TEXT("baseConfirmationSemaphore"));	//Semaphore name
	if (hUpdateBaseEvent != NULL) {
		CopyGameData();
		while (1) {
			WaitForSingleObject(hUpdateBaseEvent, INFINITE);
			CompareAndInvalidateBases(*hGameWnd);
			UpdateWindow(*hGameWnd);
			CopyGameData();	//Can only copy after updating, because "InvalidateRect()" takes a pointer to RECT, not a copy of RECT
			ReleaseSemaphore(hBaseConfirmationSemaphore, 1, NULL);	//"Send" confirmation to server that this client has stored the current state of the game
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
		UpdateBasesThread,	//Function to execute
		(LPVOID)hGameWnd,	//Function param
		0,	//The thread runs immediately after creation
		NULL	//Thread ID is not stored anywhere
	);
	return hReadUpdatedBaseThread != NULL;
}
DLL_API void PrintGameData(HDC hdc) {
	RECT rectangle;
	HBRUSH hBrushIn = CreateSolidBrush(RGB(255, 0, 0));
	HBRUSH hBrushOut = CreateSolidBrush(RGB(0, 0, 0));
	HBRUSH hBrushDeleted = CreateSolidBrush(GetDCBrushColor(hdc));

	if (firstPaint == TRUE) {
		//Initial paint
		for (int i = 0; i < MAX_BLOCKS; i++) {
			FillRect(hdc, &gameDataStart->block[i].rectangle, hBrushIn);
			FrameRect(hdc, &gameDataStart->block[i].rectangle, hBrushOut);
		}
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (gameDataStart->base[i].hasPlayer) {
				FillRect(hdc, &gameDataStart->base[i].rectangle, hBrushIn);
				FrameRect(hdc, &gameDataStart->base[i].rectangle, hBrushOut);
			}
		}
		for (int i = 0; i < MAX_BALLS; i++) {
			FillRect(hdc, &gameDataStart->ball[i].rectangle, hBrushIn);
			FrameRect(hdc, &gameDataStart->ball[i].rectangle, hBrushOut);
		}
		firstPaint = FALSE;
	}
	else {
		//Game has started
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (cachedGameData.base[i].hasPlayer && cachedGameData.base[i].changed) {
				FillRect(hdc, &cachedGameData.base[i].rectangle, hBrushIn);
				FrameRect(hdc, &cachedGameData.base[i].rectangle, hBrushOut);
				FillRect(hdc, &deletedBasePart, hBrushDeleted);
				FrameRect(hdc, &deletedBasePart, hBrushDeleted);
				
			}
		}
		for (int i = 0; i < MAX_BALLS; i++) {
			if (gameDataStart->ball[i].isActive) {
				FillRect(hdc, &cachedGameData.ball[i].rectangle, hBrushIn);
				FrameRect(hdc, &cachedGameData.ball[i].rectangle, hBrushOut);
			}
		}
	}
}
//(SERVER INITIALIZATION)
BOOL LoadGameResources() {
	return (LoadGameMappedFileResources() &&
		LoadNewUserResources() &&
		LoadPlayerMsgResources());
}