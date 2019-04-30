#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include "Resources.h"

_ball ball;
_gameArea gameArea;

void InitGameSettings(int gameAreaWidth, int gameAreaHeight) {
	//Game Area
	gameArea.width = gameAreaWidth;				// [0, gameAreaWidth[
	gameArea.height = gameAreaHeight;			// [0, gameAreaHeight[
	//Ball
	ball.direction = topRight;
	ball.coordinates.x = (gameArea.width/2)-1;	//horizontal-center
	ball.coordinates.y = gameArea.height-1;		//vertical-bottom

}

void LoadMessageFileView(HANDLE hMessageFV) {

}

void LoadDataFileView(HANDLE hDataFV) {

}

BOOL LoadSharedInfo() {
	//CLOSE HANDLES IN THE END
	HANDLE hFile = CreateFile(_T("../Server/SharedInfo.txt"),	//File name
		GENERIC_READ | GENERIC_WRITE,							//Access
		FILE_SHARE_READ | FILE_SHARE_WRITE,						//Share mode
		NULL,													//Security attributes
		OPEN_EXISTING,											//Creation disposition
		FILE_ATTRIBUTE_NORMAL,									//Flags and attributes
		NULL);													//Template File (not used)
	if (hFile != INVALID_HANDLE_VALUE) {
		_tprintf(_T("Successfully opened file\n"));
		//NOTE #1: "An attempt to map a file with a length of 0 (zero) fails with an error code of ERROR_FILE_INVALID. Applications should test for files with a length of 0 (zero) and reject those files."
		//SRC #1: https://docs.microsoft.com/en-us/windows/desktop/api/WinBase/nf-winbase-createfilemappinga
		//NOTE #2: I chose 4096 bytes for the size, because it's the cluster size, 4096 bytes is enough, and the way I'm not uselessly wasting disk space, later on when saving the file.
		//SRC #2: https://superuser.com/questions/66825/what-is-the-difference-between-size-and-size-on-disk
		HANDLE hFileMapping = CreateFileMapping(hFile,
			NULL,
			PAGE_READWRITE,
			0,													//Size in bytes	(high-order)
			4096,												//Size in bytes (low-order)
			_T("LocalSharedInfo"));
		
		if (hFileMapping != NULL) {
			_tprintf(_T("Successfully mapped file in memory\n"));
			return TRUE;
		}
		else {
			_tprintf(_T("ERROR MAPPING FILE. Exiting...\n"));
			DWORD errorCode = GetLastError();
			_tprintf(_T("%x"), errorCode);
			_tprintf(_T("%x"), errorCode);
		}
	}
	else
		_tprintf(_T("ERROR OPENING FILE. Exiting...\n"));

	return FALSE;
	//LoadDataFileView();
	//LoadMessageFileView();
}

int _tmain(int argc, const _TCHAR* argv[]) {
	_tprintf(_T("Executable location: %s\n"), argv[0]);
	if (LoadSharedInfo()) {
		_TCHAR cmd[CMD_SIZE];
		while (_tcscmp(cmd, _T("exit"))) {
			_tprintf(_T("Command: "));
			_tscanf_s(_T("%s"), cmd, CMD_SIZE - 1);
		}
	}
	return 0;
}