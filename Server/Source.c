#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include "Resources.h"
 
//Game vars
_ball ball;
_gameArea gameArea;
_gameSettings gameSettings;
//File mapping vars
HANDLE hFile;
HANDLE hFileMapping;
LPVOID dataFileView, messageFileView;


//File Mapping
LPVOID LoadFileView(int offset, int size) {
	return MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ | FILE_MAP_WRITE,
		0,
		offset,
		size);
}
BOOL LoadSharedInfo(LPCSTR fileName) {
	//SRC #1: https://docs.microsoft.com/pt-pt/windows/desktop/api/fileapi/nf-fileapi-createfilea
	hFile = CreateFile(_T(fileName),								//File name
		GENERIC_READ | GENERIC_WRITE,								//Access
		FILE_SHARE_READ | FILE_SHARE_WRITE,							//Share mode
		NULL,														//Security attributes
		OPEN_EXISTING,												//Creation disposition
		FILE_ATTRIBUTE_NORMAL,										//Flags and attributes
		NULL);														//Template File (not used)
	if (hFile != INVALID_HANDLE_VALUE) {
		_tprintf(_T("Successfully opened file\n"));
		//SRC #2: https://docs.microsoft.com/en-us/windows/desktop/api/WinBase/nf-winbase-createfilemappinga
		hFileMapping = CreateFileMapping(hFile,
			NULL,
			PAGE_READWRITE,
			0,														//Size in bytes	(high-order)
			65536*2,												//Size in bytes (low-order)
			_T("LocalSharedInfo"));
		
		if (hFileMapping != NULL) {
			_tprintf(_T("Successfully mapped file in memory\n"));
			if ((dataFileView = LoadFileView(0, 65536)) != NULL && (messageFileView = LoadFileView(65536, 65536)) != NULL) {
				_tprintf(_T("Successfully created file views\n"));
				return TRUE;
			} else {
				DWORD errorCode = GetLastError();
				_tprintf(_T("%x"), errorCode);
			}
		} else {
			_tprintf(_T("ERROR MAPPING FILE. Exiting...\n"));
			DWORD errorCode = GetLastError();
			_tprintf(_T("%x"), errorCode);
		}
	} else
		_tprintf(_T("ERROR OPENING FILE. Exiting...\n"));
	return FALSE;
}
void CloseSharedInfoHandles() {
	UnmapViewOfFile(messageFileView);
	UnmapViewOfFile(dataFileView);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);
}

//Game
void InitGameSettings(int gameAreaWidth, int gameAreaHeight) {
	//Game Area
	gameArea.width = gameAreaWidth;				// [0, gameAreaWidth[
	gameArea.height = gameAreaHeight;			// [0, gameAreaHeight[
												//Ball
	ball.direction = topRight;
	ball.coordinates.x = (gameArea.width / 2) - 1;	//horizontal-center
	ball.coordinates.y = gameArea.height - 1;		//vertical-bottom

}
void Help() {
	_tprintf(_T("Available commands: \n"));
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
	} while (_tcscmp(cmd, _T("exit")) != 0);
}

//Main
int _tmain(int argc, const _TCHAR* argv[]) {
	_tprintf(_T("Arkanoid server:\nExecutable location: %s\n-------------------------------------------\nLoading...\n"), argv[0]);
	if (LoadSharedInfo(_T("../Server/SharedInfo.txt"))) {
		CmdLoop();
		CloseSharedInfoHandles();
	}
	return 0;
}