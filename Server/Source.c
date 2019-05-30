#include <stdio.h>
#include <time.h> 
#include <tchar.h>
#include <Windows.h> 
#include "Resources.h"
 
//File mapping vars
HANDLE hFileMapping;
HANDLE hThreadBall;
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
	/*hFile = CreateFile(MAPPED_FILE_NAME,		//File name
		GENERIC_READ | GENERIC_WRITE,			//Access
		FILE_SHARE_READ | FILE_SHARE_WRITE,		//Share mode
		NULL,									//Security attributes
		OPEN_EXISTING,							//Creation disposition
		FILE_ATTRIBUTE_NORMAL,					//Flags and attributes
		NULL);									//Template File (not used)
	if (hFile != INVALID_HANDLE_VALUE) {*/
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		//_tprintf(_T("Successfully opened file: %s\n"), MAPPED_FILE_NAME);
		//SRC #2: https://docs.microsoft.com/en-us/windows/desktop/api/WinBase/nf-winbase-createfilemappinga
		hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
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
	/*} else
		_tprintf(_T("ERROR OPENING FILE: %s\n"), MAPPED_FILE_NAME);*/
	_tprintf(_T("Error code: 0x%x\n"), GetLastError());
	return FALSE;
}
VOID CloseSharedInfoHandles() {
	UnmapViewOfFile(messageStart);
	UnmapViewOfFile(dataStart);
	CloseHandle(hFileMapping);
	//CloseHandle(hFile);
}
VOID WriteAllToDataFileView(_gameSettings gameSettings, _block* block, _ball* ball) {
	/*Write order:
	 * client offset(para usar na meta 2 em View of file - Messages)
	 * game settings
	 * blocks
	 * balls
	 * perks
	 * bases
	 * clients
	*/
	dataIterator = dataStart;
	*((INT*)dataIterator) = 0;
	dataIterator = (INT*)dataIterator + 1;

	*((_gameSettings*)dataIterator) = gameSettings;
	dataIterator = (_gameSettings*)dataIterator + 1;

	for (INT i = 0; i < gameSettings.totalBlocks; i++) {
		*((_block*)dataIterator) = block[i];
		dataIterator = (_block*)dataIterator + 1;
	}

	for (INT i = 0; i < gameSettings.maxBalls; i++) {
		*((_ball*)dataIterator) = ball[i];
		dataIterator = (_ball*)dataIterator + 1;
	}

	/*TO DO
	for (INT i = 0; i < gameSettings.maxPerks; i++) {
		*((_perk*)dataIterator) = perk[i];
		dataIterator = (_perk*)dataIterator + 1;
	}*/

}
VOID WriteToDataFileView(INT type, LPVOID val, INT offset, _gameSettings gameSettings) {
	dataIterator = dataStart;
	switch (type) {
		case 0: *((INT*)dataIterator) = *((INT*)val); break;	//Client_offset	(SEMAPHORE/MUTEX HERE)
		case 1: {
			dataIterator = (INT*)dataIterator + 1;
			*((_gameSettings*)dataIterator) = *((_gameSettings*)val);
		} break;
		case 2: {
			dataIterator = (INT*)dataIterator + 1;
			dataIterator = (_gameSettings*)dataIterator + 1;
			dataIterator = (_block*)dataIterator + offset;
			*((_block*)dataIterator) = *((_block*)val);
			
		} break;
		case 3: {
			dataIterator = (INT*)dataIterator + 1;
			dataIterator = (_gameSettings*)dataIterator + 1;
			dataIterator = (_block*)dataIterator + gameSettings.totalBlocks;
			dataIterator = (_ball*)dataIterator + offset;
			*((_ball*)dataIterator) = *((_ball*)val);
		} break;
		/* TO DO
		case 4: {
			dataIterator = (INT*)dataIterator + 1;
			dataIterator = (_gameSettings*)dataIterator + 1;
			dataIterator = (_block*)dataIterator + gameSettings.totalBlocks;
			dataIterator = (_ball*)dataIterator + gameSettings.maxBalls;
			*((_perk*)dataStart) = *((_perk*)val);
		} break;
		case 5: *((_base*)dataStart) = *((_base*)val); break;
		case 6: *((_client*)dataStart) = *((_client*)val); break;*/
	}
}

//Threads -> Ball
DWORD WINAPI ThreadBall(LPVOID lpParameter) {
	//TO DO
}

//Threads -> Initializations
BOOL InitThreadBall() {
	hThreadBall = CreateThread(
		NULL,				//hThreadUsers cannot be inherited by child processes
		0,					//Default stack size
		ThreadBall,			//Function to execute
		NULL,				//Function param
		CREATE_SUSPENDED,	//Wait for game to start, to start thread execution
		NULL				//Thread ID is not stored anywhere
	);
	if (hThreadBall != NULL)
		return TRUE;
	_tprintf(_T("ERROR CREATING THREAD: 'hThreadBall'\n"));
	return FALSE;
}
BOOL InitThreads() {
	if (InitThreadBall()) {
		_tprintf(_T("All threads created successfully. [NEED TO INITIALIZE USER THREAD STILL]\n"));
		return TRUE;
	}
	_tprintf(_T("Error code: 0x%x\n"), GetLastError());
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

//Game -> Blocks
VOID InitBlocksrray(_block* block, INT size) {
	for (INT i = 0; i < size; i++) {
		block[i].coordinates.x = -1;
		block[i].coordinates.y = -1;
		block[i].type = normal;
	}
}
BOOL BlockIsInPos(INT x, INT y, _block* block, INT size) {
	for (INT i = 0; i < size; i++) {
		if (block[i].coordinates.x == x && block[i].coordinates.y == y)
			return TRUE;
	}
	return FALSE;
}
BOOL GenerateMap(UINT seed, _gameSettings* gameSettings, _block** block) {
	//Regras para gerar blocos pseudo-aleatóriamente:
	// (DONE) - Regra #1: Os blocos só podem aparecer a uma distancia >= (2*ball.size) dos edges do mapa
	// (DONE) - Regra #2: Os blocos só podem existir em 75% da altura do mapa, a contar de cima para baixo
	// (DONE) - Regra #3: Todos os blocos têm as mesmas dimensões
	// (DONE) - Regra #4: Blocos seguidos não têm espaçamentos entre si

	srand(seed);

	INT borderPadding = gameSettings->defaultBallSize * 2,
		usableWidth = (INT)(gameSettings->dimensions.width - borderPadding * 2),
		usableHeight = (INT)((gameSettings->dimensions.height - borderPadding * 2)*0.75),
		maxBlocksPerLine = (INT)(usableWidth / gameSettings->blockDimensions.width),
		maxBlockLines = (INT)(usableHeight / gameSettings->blockDimensions.height),
		maxBlocks = maxBlocksPerLine * maxBlockLines,
		posX,
		posY;

	gameSettings->totalBlocks = ((INT)(maxBlocks*0.5)) + (rand() % ((INT)(maxBlocks*0.3)));	//No minimo, 50% dos espaço disponivel para os blocos (75% da area de jogo), é garantido estar ocupado. No máximo, 80% está ocupado
	(*block) = (_block*)malloc(gameSettings->totalBlocks * sizeof(_block));
	if ((*block) == NULL) {
		_tprintf(_T("ERROR LOADING BLOCKS ARRAY\n"));
		return FALSE;
	}
	else {
		InitBlocksrray((*block), gameSettings->totalBlocks);
		for (INT i = 0; i < gameSettings->totalBlocks; i++) {
			do {
				posX = borderPadding + ((rand() % maxBlocksPerLine) * gameSettings->blockDimensions.width);
				posY = borderPadding + ((rand() % maxBlockLines) * gameSettings->blockDimensions.height);
			} while (BlockIsInPos(posX, posY, (*block), i));	//Aqui uso "i" em vez do tamanho do array, porque nao vale a pena verificar espaços vazios do array
			(*block)[i].coordinates.x = posX;
			(*block)[i].coordinates.y = posY;
		}
		return TRUE;
	}
}

//Game -> Initializations
BOOL InitializeEmptyTopTen(PHKEY topTenKey) {
	//NEW REGISTRY KEY	(initizlizes all positions to -1)
	INT defaultValue = 0;
	_TCHAR name[] = "1";
	LSTATUS valueStatus;
	for (int i = 0; i < 9; i++) {
		if ((valueStatus = RegSetValueEx(*topTenKey, name, 0, REG_DWORD, &defaultValue, sizeof(INT))) == ERROR_SUCCESS)	//inicializa as primeiras 9 posições
			name[0]++;
		else {
			_tprintf(_T("ERROR INITIALIZING VALUE NAMED: %s\nError code: %ld\n"), name, valueStatus);
			return FALSE;
		}
	}
 	if ((valueStatus = RegSetValueEx(*topTenKey, "10", 0, REG_DWORD, &defaultValue, sizeof(INT))) != ERROR_SUCCESS)	{	//inicializa 10ª posição
		_tprintf(_T("ERROR INITIALIZING VALUE: 10\nError code: %ld\n"), valueStatus);
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
BOOL LoadClientsArray(_client** player, INT maxClients) {
	(*player) = (_client*)malloc(maxClients * sizeof(_client));
	if ((*player) == NULL) {
		_tprintf(_T("ERROR LOADING CLIENTS ARRAY\n"));
		return FALSE;
	}
	else {
		for (INT i = 0; i < maxClients; i++) {
			(*player)->id = -1;
			memset((*player)->username, 0, USERNAME_MAX_LENGHT);
		}
		return TRUE;
	}
}
BOOL LoadGameSettings(_TCHAR* fileName, _gameSettings* gameSettings) {
	FILE *fp;
	if (_tfopen_s(&fp, fileName, _T("r")) == 0) {
		_tprintf(_T("Successfully opened file: %s\n"), fileName);

		_TCHAR val[10];
		_fgetts(val, 10, fp);
		gameSettings->maxPlayers = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->maxSpectators = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->maxBalls = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->maxPerks = _tstoi(val);
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
		_fgetts(val, 10, fp);
		gameSettings->blockDimensions.width = _tstoi(val);
		_fgetts(val, 10, fp);
		gameSettings->blockDimensions.height = _tstoi(val);

		gameSettings->hasStarted = FALSE;

		fclose(fp);
		return TRUE;
	}
	else
		_tprintf(_T("ERROR OPENING FILE: %s\n"), fileName);
	return FALSE;
}
BOOL Initialize(_TCHAR* defaultsFileName, _gameSettings* gameSettings, _ball** ball, _block** block, _client** player, PHKEY topTenKey) {
	if (LoadSharedInfo() &&
		LoadGameSettings(defaultsFileName, gameSettings) &&
		LoadBallsArray(ball, gameSettings->maxBalls, gameSettings->defaultBallSpeed, gameSettings->defaultBallSize, gameSettings->dimensions.width, gameSettings->dimensions.height) &&
		GenerateMap(time(0), gameSettings, block) &&
		LoadClientsArray(player, gameSettings->maxPlayers) &&
		LoadTopTen(topTenKey) &&
		InitThreads())
		return TRUE;
	return FALSE;
}

//Game -> Command
VOID Help() {
	_tprintf(_T("-------------------------------------------\n"));
	_tprintf(_T("Available commands: \n"));
	_tprintf(_T("start -> Starts the game for connected clients.\n"));
	_tprintf(_T("newMap -> Generates a new map if the game hasn't started yet.\n"));
	_tprintf(_T("top10 -> Lists top 10 scores.\n"));
	_tprintf(_T("exit -> Orderly closes the server.\n"));
}
VOID Start() {
	if (ResumeThread(hThreadBall) != -1)
		_tprintf(_T("Successfully initialized thread: 'ThreadBall'\n"));
	else
		_tprintf(_T("Error starting game.\nError code: 0x%x\n"), GetLastError());
}
VOID NewMap(_gameSettings* gameSettings, _block** block) {
	free((*block));
	if(GenerateMap(time(0), gameSettings, block))
		_tprintf(_T("Successfully generated new map\n"));
}
VOID ShowTop10(PHKEY topTenKey) {
	INT data;		
	_TCHAR name[] = "1";
	LSTATUS valueStatus;
	DWORD pdwType;
	DWORD pcbData;
	_tprintf(_T("Top 10:\n"));
	for (int i = 0; i < 9; i++) {
		if ((valueStatus = RegGetValueA(*topTenKey, NULL, name, RRF_RT_REG_DWORD, &pdwType, &data, &pcbData)) == ERROR_SUCCESS) {	//inicializa as primeiras 9 posições
			_tprintf(_T(" #%d: %d\n"), i+1, data);
			name[0]++;
		}
		else {
			_tprintf(_T("ERROR GETTING VALUE: %s\nError code: %ld\n"), name, valueStatus);
			return FALSE;
		}
	}
	if ((valueStatus = RegGetValueA(*topTenKey, NULL, "10", RRF_RT_REG_DWORD, &pdwType, &data, &pcbData)) == ERROR_SUCCESS)	//inicializa as primeiras 9 posições
		_tprintf(_T(" #%d: %d\n"), 10, data);
	else {
		_tprintf(_T("ERROR GETTING VALUE: 10\nError code: %ld\n"), name, valueStatus);
		return FALSE;
	}
}
VOID CmdLoop(_gameSettings* gameSettings, _block** block, PHKEY topTenKey) {
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
		else if (_tcscmp(cmd, _T("newMap")) == 0)
			NewMap(gameSettings, block);
		else if (_tcscmp(cmd, _T("top10")) == 0)
			ShowTop10(topTenKey);
	} while (_tcscmp(cmd, _T("exit")) != 0);
}

//Dynamic Memory Management
VOID DeallocDynamicMemory(_ball** ball, _block** block, _client** player) {
	free((*ball));
	free((*block));
	free((*player));
}
VOID CleanUp(_ball** ball, _block** block, _client** player) {
	CloseSharedInfoHandles();
	DeallocDynamicMemory(ball, block, player);
}

//Main
INT _tmain(INT argc, const _TCHAR* argv[]) {
	if (argc == 2) {
		//Game vars
		_gameSettings gameSettings;
		_ball* ball = NULL;
		_block* block = NULL;
		_client* player = NULL;
		INT spectators = 0;

		HKEY topTenKey;
		_tprintf(_T("Arkanoid server:\nExecutable location: %s\n-------------------------------------------\nInitializing...\n"), argv[0]);
		if (Initialize(argv[1], &gameSettings, &ball, &block, &player, &topTenKey)) {
			_tprintf(_T("Done!\n"));
			CmdLoop(&gameSettings, &block, &topTenKey);
			CleanUp(&ball, &block, &player);
		}
		else {
			_tprintf(_T("-------------------------------------------\nPress ENTER to exit."));
			getchar();
		}
	}
	return 0;
}