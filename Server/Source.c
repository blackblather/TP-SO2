#include <stdio.h>
#include <tchar.h>
#include <Windows.h> 
#include "Resources.h"
 
//Game vars
_ball ball;
_gameSettings gameSettings;

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
		_tprintf(_T("Successfully opened file: %s\n"), MAPPED_FILE_NAME);
		//SRC #2: https://docs.microsoft.com/en-us/windows/desktop/api/WinBase/nf-winbase-createfilemappinga
		hFileMapping = CreateFileMapping(hFile,
			NULL,
			PAGE_READWRITE,
			0,									//Size in bytes	(high-order)
			65536*2,							//Size in bytes (low-order)
			_T("LocalSharedInfo"));
		
		if (hFileMapping != NULL) {
			_tprintf(_T("Successfully mapped file in memory\n"));
			if ((dataStart = LoadFileView(0, 65536)) != NULL && (messageStart = LoadFileView(65536, 65536)) != NULL) {
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

//Game -> GameSettings
BOOL InitializeEmptyTopTen(PHKEY topTenKey) {
	//NEW REGISTRY KEY	(initizlizes all positions to -1)
	INT defaultValue = -1;
	_TCHAR name[] = "1";
	LSTATUS valueStatus;
	for (int i = 0; i < 9; i++) {
		if ((valueStatus = RegSetValueExA(*topTenKey, name, 0, REG_DWORD, &defaultValue, sizeof(defaultValue))) == ERROR_SUCCESS)	//inicializa as primeiras 9 posições
			name[0]++;
		else {
			_tprintf(_T("ERROR INITIALIZING VALUE NAMED: %s\nError code: %l\n", name, valueStatus));
			return FALSE;
		}
	}
	if ((valueStatus = RegSetValueExA(*topTenKey, "10", 0, REG_DWORD, &defaultValue, sizeof(defaultValue))) == ERROR_SUCCESS)		//inicializa 10ª posição
		name[0]++;
	else {
		_tprintf(_T("ERROR INITIALIZING VALUE NAMED: %s\nError code: %l\n", name, valueStatus));
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
		_tprintf(_T("ERROR OPENING REGKEY\nError code: %l\n", keyStatus));
	return FALSE;
}
BOOL LoadDefaults(_TCHAR* fileName) {
	FILE *fp;
	if (_tfopen_s(&fp, fileName, _T("r")) == 0) {
		_tprintf(_T("Successfully opened file: %s\n"), fileName);

		//Game Area
		_TCHAR val[10];
		_fgetts(val, 10, fp);
		gameSettings.maxPlayers = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.levels = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.speedUps = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.slowDowns = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.duration = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.speedUpChance = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.slowDownChance = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.lives = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.width = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings.height = _tstoi(val);

		//Ball
		ball.direction = topRight;
		ball.coordinates.x = (gameSettings.width / 2) - 1;	//horizontal-center
		ball.coordinates.y = gameSettings.height - 1;		//vertical-bottom

		fclose(fp);
		return TRUE;
	}
	else
		_tprintf(_T("ERROR OPENING FILE: %s\n"), fileName);
	return FALSE;
}
BOOL LoadGameSettings(_TCHAR* fileName, PHKEY topTenKey) {
	if (LoadDefaults(fileName) && LoadTopTen(topTenKey))
		return TRUE;
	return FALSE;
}
//Game -> Command
void ShowTop10(PHKEY topTenKey) {

}
void Help() {
	_tprintf(_T("Available commands: \n"));
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
		else if (_tcscmp(cmd, _T("showTop10")) == 0)
		{

		}
	} while (_tcscmp(cmd, _T("exit")) != 0);
}

//Main
int _tmain(int argc, const _TCHAR* argv[]) {
	if (argc == 2) {
		HKEY topTenKey;
		_tprintf(_T("Arkanoid server:\nExecutable location: %s\n-------------------------------------------\nInitializing...\n"), argv[0]);
		if (LoadSharedInfo() && LoadGameSettings(argv[1], &topTenKey)) {
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