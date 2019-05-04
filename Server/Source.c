#include <stdio.h>
#include <tchar.h>
#include <Windows.h> 
#include "Resources.h"
 
//File mapping vars
HANDLE hFile;
HANDLE hFileMapping;
LPVOID dataStart, dataIterator, messageStart, messageIterator;


//File Mapping
LPVOID LoadFileView(int offset, int size) {
	return MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ | FILE_MAP_WRITE,
		0,
		offset,
		size);
}
BOOL LoadSharedInfo() {
	//SRC #1: https://docs.microsoft.com/pt-pt/windows/desktop/api/fileapi/nf-fileapi-createfilea
	hFile = CreateFile(MAPPED_FILE_NAME,		//File name
		GENERIC_READ | GENERIC_WRITE,			//Access
		FILE_SHARE_READ | FILE_SHARE_WRITE,		//Share mode
		NULL,									//Security attributes
		OPEN_EXISTING,							//Creation disposition
		FILE_ATTRIBUTE_NORMAL,					//Flags and attributes
		NULL);									//Template File (not used)
	if (hFile != INVALID_HANDLE_VALUE) {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		_tprintf(_T("Successfully opened file: %s\n"), MAPPED_FILE_NAME);
		//SRC #2: https://docs.microsoft.com/en-us/windows/desktop/api/WinBase/nf-winbase-createfilemappinga
		hFileMapping = CreateFileMapping(hFile,
			NULL,
			PAGE_READWRITE,
			0,									//Size in bytes	(high-order)
			sysInfo.dwAllocationGranularity*2,							//Size in bytes (low-order)
			_T("LocalSharedInfo"));
		
		if (hFileMapping != NULL) {
			_tprintf(_T("Successfully mapped file in memory\n"));
			if ((dataStart = LoadFileView(0, sysInfo.dwAllocationGranularity)) != NULL && (messageStart = LoadFileView(sysInfo.dwAllocationGranularity, sysInfo.dwAllocationGranularity)) != NULL) {
				_tprintf(_T("Successfully created file views\n"));
				dataIterator = dataStart;
				messageIterator = messageStart;
				return TRUE;
			} else
				_tprintf(_T("ERROR CREATING FILE VIEWS.\n"));
		} else
			_tprintf(_T("ERROR MAPPING FILE.\n"));
	} else
		_tprintf(_T("ERROR OPENING FILE: %s\n"), MAPPED_FILE_NAME);
	_tprintf(_T("Error code: 0x%x\n"), GetLastError());
	return FALSE;
}
void CloseSharedInfoHandles() {
	UnmapViewOfFile(messageStart);
	UnmapViewOfFile(dataStart);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);
}

//Threads -> Ball
DWORD WINAPI ThreadBall(LPVOID lpParameter) {
}
BOOL InitThreadBall() {
	HANDLE hThreadBall = CreateThread(
		NULL,			//hThreadUsers cannot be inherited by child processes
		0,				//Default stack size
		ThreadBall,		//Function to execute
		NULL,			//Function param
		0,				//Runs imediatly
		NULL			//Thread ID is not stored anywhere
	);
	if (hThreadBall == NULL) {
		_tprintf(_T("ERROR CREATING THREAD: 'hThreadBall'\n"));
		return FALSE;
	}
	return TRUE;
}
//Threads
BOOL InitThreads() {
	_tprintf(_T("All threads initialized successfully. [NEED TO INITIALIZE USER THREAD STILL]\n"));
	return TRUE;
	//_tprintf(_T("Error code: 0x%x\n"), GetLastError());
	//return FALSE;
}

//Game -> Initializations
BOOL InitializeEmptyTopTen(PHKEY topTenKey) {
	//NEW REGISTRY KEY	(initizlizes all positions to -1)
	INT defaultValue = 0;		//8bit value -> range [0, 255]
	_TCHAR name[] = "1";
	LSTATUS valueStatus;
	for (int i = 0; i < 9; i++) {
		if ((valueStatus = RegSetValueEx(*topTenKey, name, 0, REG_DWORD, &defaultValue, sizeof(defaultValue))) == ERROR_SUCCESS)	//inicializa as primeiras 9 posições
			name[0]++;
		else {
			_tprintf(_T("ERROR INITIALIZING VALUE NAMED: %s\nError code: %ld\n"), name, valueStatus);
			return FALSE;
		}
	}
 	if ((valueStatus = RegSetValueEx(*topTenKey, "10", 0, REG_DWORD, &defaultValue, sizeof(defaultValue))) == ERROR_SUCCESS)		//inicializa 10ª posição
		name[0]++;
	else {
		_tprintf(_T("ERROR INITIALIZING VALUE NAMED: %s\nError code: %ld\n"), name, valueStatus);
		return FALSE;
	}
	return TRUE;
}
BOOL LoadTopTen(PHKEY topTenKey) {
	DWORD disposition;
	LSTATUS keyStatus = RegCreateKeyEx(
		HKEY_CURRENT_USER,
		_T("Arkanoid_Top_Ten"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_SET_VALUE | KEY_QUERY_VALUE,
		NULL,
		topTenKey,
		&disposition);
	if (keyStatus == ERROR_SUCCESS) {
		if (disposition == REG_CREATED_NEW_KEY) {
			_tprintf(_T("Created and opened new RegKey: HKEY_CURRENT_USER\\Arkanoid_Top_Ten\n"));
			if (!InitializeEmptyTopTen(topTenKey))
				return FALSE;
		}
		else
			_tprintf(_T("Opened existing RegKey: HKEY_CURRENT_USER\\Arkanoid_Top_Ten\n"));
		return TRUE;
	} else
		_tprintf(_T("ERROR OPENING REGKEY\nError code: %ld\n"), keyStatus);
	return FALSE;
}
BOOL LoadBallsArray(_ball** ball, INT maxBalls, INT speed, INT size, INT gameAreaWidth, INT gameAreaHeight) {
	(*ball) = (_ball*)malloc(maxBalls * sizeof(_ball));
	if ((*ball) == NULL) {
		_tprintf(_T("ERROR LOADING BALLS ARRAY\n"));
		return FALSE;
	}
	else {
		for (INT i = 0; i < maxBalls; i++) {
			(*ball)[i].direction = topRight;
			(*ball)[i].size = size;
			(*ball)[i].coordinates.x = (gameAreaWidth/2) - ((*ball)[i].size/2);
			(*ball)[i].coordinates.y = (gameAreaHeight - 50);
			(*ball)[i].speed = speed;
			(*ball)[i].isActive = FALSE;
		}
		return TRUE;
	}
}
BOOL LoadGameSettings(_TCHAR* fileName, _gameSettings* gameSettings, _ball** ball) {
	FILE *fp;
	if (_tfopen_s(&fp, fileName, _T("r")) == 0) {
		_tprintf(_T("Successfully opened file: %s\n"), fileName);

		_TCHAR val[10];
		_fgetts(val, 10, fp);
		gameSettings->maxPlayers = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->maxBalls = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->defaultBallSpeed = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->defaultBallSize = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->levels = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->speedUps = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->slowDowns = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->duration = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->speedUpChance = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->slowDownChance = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->lives = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->dimensions.width = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->dimensions.height = _tstoi(val);

		fclose(fp);
		return TRUE;
	}
	else
		_tprintf(_T("ERROR OPENING FILE: %s\n"), fileName);
	return FALSE;
}
BOOL Initialize(_TCHAR* defaultsFileName, _gameSettings* gameSettings, _ball** ball, PHKEY topTenKey) {
	if (LoadSharedInfo() &&
		LoadGameSettings(defaultsFileName, gameSettings, ball) &&
		LoadBallsArray(ball, gameSettings->maxBalls, gameSettings->defaultBallSpeed, gameSettings->defaultBallSize, gameSettings->dimensions.width, gameSettings->dimensions.height) &&
		LoadTopTen(topTenKey) &&
		InitThreads())
		return TRUE;
	return FALSE;
}
//Game -> Balls
INT GetActiveBalls(_ball* ball, INT maxBalls) {
	INT total = 0;
	for (INT i = 0; i < maxBalls; i++)
		if (ball[i].isActive)
			total++;
	return total;
}
//Game -> Command
void Start() {
	if (InitThreadBall())
		_tprintf(_T("Successfully initialized thread: 'ThreadBall'\n"));
	else
		_tprintf(_T("\n"));
}
void ShowTop10(PHKEY topTenKey) {

}
void Help() {
	_tprintf(_T("Available commands: \n"));
	_tprintf(_T("start -> Starts the game for connected clients.\n"));
	_tprintf(_T("showTop10 -> Lists top 10 scores.\n"));
	_tprintf(_T("exit -> Orderly closes the server.\n"));
}
void CmdLoop() {
	_TCHAR cmd[CMD_SIZE];
	_tprintf(_T("-------------------------------------------\n"));
	_tprintf(_T("Type \"help\" to list available commands.\n"));
	do {
		_tprintf(_T("-------------------------------------------\n"));
		_tprintf(_T("Command: "));
		_tscanf_s(_T("%s"), cmd, CMD_SIZE - 1);
		if (_tcscmp(cmd, _T("help")) == 0)
			Help();
		else if (_tcscmp(cmd, _T("start")) == 0)
			Start();
		else if (_tcscmp(cmd, _T("showTop10")) == 0)
		{

		}
	} while (_tcscmp(cmd, _T("exit")) != 0);
}

//Main
int _tmain(int argc, const _TCHAR* argv[]) {
	if (argc == 2) {
		//Game vars
		_gameSettings gameSettings;
		_ball* ball;
		HKEY topTenKey;
		_tprintf(_T("Arkanoid server:\nExecutable location: %s\n-------------------------------------------\nInitializing...\n"), argv[0]);
		if (Initialize(argv[1], &gameSettings, &ball, &topTenKey)) {
			_tprintf(_T("Done!\n"));
			CmdLoop();
			CloseSharedInfoHandles();
		}
		else {
			_tprintf(_T("-------------------------------------------\nPress ENTER to exit."));
			getchar();
		}
	}
	return 0;
}