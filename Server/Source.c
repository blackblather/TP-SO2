#include <stdio.h>
#include <time.h> 
#include <tchar.h>
#include <Windows.h> 
#include "Resources.h"
 
//Server specific structs
struct newUsersParam_STRUCT {
	_gameSettings* gameSettings;
	_gameMsgNewUser* gameMsgNewUser;
	_client* player;
	int* loggedInPlayers;
} typedef _newUsersParam;

//Client vars
HANDLE hPlayerMutex;		//To use "player" array and/or "loggedInPlayers", use this mutex
//Game Settings Mutex
HANDLE hGameSettingsMutex;

//File Mapping
LPVOID LoadFileView(HANDLE hFileMapping, int offset, int size) {
	return MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ | FILE_MAP_WRITE,
		0,
		offset,
		size);
}
BOOL LoadSharedInfo(HANDLE* hFileMapping, LPVOID* messageBaseAddr, _gameData** gameDataStart, _gameMsgNewUser** gameMsgNewUser, _serverResponse** serverResp, _clientMsg** messageStart) {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	//SRC #2: https://docs.microsoft.com/en-us/windows/desktop/api/WinBase/nf-winbase-createfilemappinga
	(*hFileMapping) = CreateFileMapping(INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,									//Size in bytes	(high-order)
		sysInfo.dwAllocationGranularity*2,	//Size in bytes (low-order)
		_T("LocalSharedInfo"));
		
	if ((*hFileMapping) != NULL) {
		_tprintf(_T("Successfully mapped file in memory\n"));
		if (((*gameDataStart) = (_gameData*) LoadFileView((*hFileMapping), 0, sysInfo.dwAllocationGranularity)) != NULL && ((*messageBaseAddr) = LoadFileView((*hFileMapping), sysInfo.dwAllocationGranularity, sysInfo.dwAllocationGranularity)) != NULL) {
			_tprintf(_T("Successfully created file views\n"));
			(*messageStart) = (_client*)(*messageBaseAddr);
			(*gameMsgNewUser) = (_gameMsgNewUser*)(*messageStart);
			(*messageStart) = (_gameMsgNewUser*)(*messageStart) + 1;
			(*serverResp) = (_serverResponse*)(*messageStart);
			(*messageStart) = (_serverResponse*)(*messageStart) + 1;
			return TRUE;
		} else
			_tprintf(_T("ERROR CREATING FILE VIEWS.\n"));
	} else
		_tprintf(_T("ERROR MAPPING FILE.\n"));
	_tprintf(_T("Error code: 0x%x\n"), GetLastError());
	return FALSE;
}
VOID CloseSharedInfoHandles(LPVOID messageBaseAddr, HANDLE hFileMapping, _gameData* gameDataStart) {
	UnmapViewOfFile(messageBaseAddr);
	UnmapViewOfFile((LPVOID)gameDataStart);
	CloseHandle(hFileMapping);
}

//Threads -> Ball
DWORD WINAPI ThreadBall(LPVOID lpParameter) {
	//TO DO
	return 1;
}

//Threads -> New Users
BOOL UsernameIsUnique(TCHAR username[USERNAME_MAX_LENGHT], _client* player) {
	int strLen = _tcsnlen(username, USERNAME_MAX_LENGHT);
	for (int i = 0; i < MAX_PLAYERS; i++)
		if (strLen != 0 && strLen != USERNAME_MAX_LENGHT)
			if (_tcscmp(player[i].username, username) == 0)
				return FALSE;
	return TRUE;
}
void AddUserToLoggedInUsersArray(TCHAR username[USERNAME_MAX_LENGHT], _client* player, int* loggedInPlayers) {
	_tcscpy_s(player[(*loggedInPlayers)].username, USERNAME_MAX_LENGHT, username);
	(*loggedInPlayers)++;
}
DWORD WINAPI ThreadNewUsers(LPVOID lpParameter) {
	//Typecast thread param
	_newUsersParam* param = (_newUsersParam*)lpParameter;

	//Create mutex object (used by clients)
	HANDLE hNewUserMutex = CreateMutex(
		NULL,					//Canoot be inherited by child processes
		FALSE,					//The server never "owns" this mutex. It's for the clients to use
		_T("newUserMutex"));	//Mutex name

	//Create event object (used by clients/server)
	HANDLE hNewUserServerEvent = CreateEvent(
		NULL,	//Security attributes
		FALSE,	//Automatic reset
		FALSE,	//Inital state = not set
		_T("newUserServerEvent"));
	//Create event object (used by clients/server)
	HANDLE hNewUserClientEvent = CreateEvent(
		NULL,	//Security attributes
		FALSE,	//Automatic reset
		FALSE,	//Inital state = not set
		_T("newUserClientEvent"));

	while (1) {
		//Wait for a client to tell the server to process his username
		WaitForSingleObject(hNewUserServerEvent, INFINITE);
			//Wait for other threads to be done using player's array and loggedInPlayers var
			WaitForSingleObject(hPlayerMutex, INFINITE);
				//Wait for other threads to be done using gameSettings struct
				//I'm avoiding using WaitForMultipleObjects() to avoid creating an array of mutexes
				WaitForSingleObject(hGameSettingsMutex, INFINITE);
					if(UsernameIsUnique(param->gameMsgNewUser->username, param->player) && (*param->loggedInPlayers) < MAX_PLAYERS) {
						if(!param->gameSettings->hasStarted)
							AddUserToLoggedInUsersArray(param->gameMsgNewUser->username, param->player, param->loggedInPlayers);
						else {
							//AddUserToSpectatorsArray();
						}
						param->gameMsgNewUser->response = TRUE;
					} else
						param->gameMsgNewUser->response = FALSE;
				ReleaseMutex(hGameSettingsMutex);
			ReleaseMutex(hPlayerMutex);
		SetEvent(hNewUserClientEvent);
	}
	return 1;
}

//Threads -> Initializations
BOOL InitThreadBall(HANDLE* hThreadBall) {
	(*hThreadBall) = CreateThread(
		NULL,				//hThreadUsers cannot be inherited by child processes
		0,					//Default stack size
		ThreadBall,			//Function to execute
		NULL,				//Function param
		CREATE_SUSPENDED,	//Wait for game to start, to start thread execution
		NULL				//Thread ID is not stored anywhere
	);
	if ((*hThreadBall) != NULL)
		return TRUE;
	_tprintf(_T("ERROR CREATING 'ball' THREAD\n"));
	return FALSE;
}
BOOL InitThreadNewUsers(HANDLE* hThreadNewUsers, _gameSettings* gameSettings, _gameMsgNewUser* gameMsgNewUser, _client* player,  int* loggedInPlayers, _newUsersParam* param) {
	param->gameSettings = gameSettings;
	param->gameMsgNewUser = gameMsgNewUser;
	param->player = player;
	param->loggedInPlayers = loggedInPlayers;
	//Create thread
	(*hThreadNewUsers) = CreateThread(
		NULL,				//hThreadNewUsers cannot be inherited by child processes
		0,					//Default stack size
		ThreadNewUsers,		//Function to execute
		param,				//Function param
		0,					//The thread runs immediately after creation
		NULL				//Thread ID is not stored anywhere
	);
	if ((*hThreadNewUsers) != NULL)
		return TRUE;
	_tprintf(_T("ERROR CREATING 'new users' THREAD\n"));
	return FALSE;
}
//BOOL InitThreadProcessPlayerMsg() {
//
//}
BOOL InitThreads(HANDLE* hThreadBall, HANDLE* hThreadNewUsers, _gameSettings* gameSettings, _gameMsgNewUser* gameMsgNewUser, _client* player, int* loggedInPlayers, _newUsersParam* param) {
	if (InitThreadBall(hThreadBall)) {
		_tprintf(_T("Initialized 'ball' thread [SUSPENDED]\n"));
		if (InitThreadNewUsers(hThreadNewUsers, gameSettings, gameMsgNewUser, player, loggedInPlayers, param)) {
			_tprintf(_T("Initialized 'new users' thread [RUNNING]\n"));
			return TRUE;
		}
	}

	//Common code if an error occurs
	_tprintf(_T("Error code: 0x%x\n"), GetLastError());
	return FALSE;
}

//GameData -> Balls
INT GetActiveBalls(_ball* ball, INT maxBalls) {
	INT total = 0;
	for (INT i = 0; i < maxBalls; i++)
		if (ball[i].isActive)
			total++;
	return total;
}
BOOL LoadBalls(_gameData* gameDataStart, INT speed, INT size, INT gameAreaWidth, INT gameAreaHeight) {
	for (INT i = 0; i < MAX_BALLS; i++) {
		gameDataStart->ball[i].direction = topRight;
		gameDataStart->ball[i].size = size;
		gameDataStart->ball[i].coordinates.x = (gameAreaWidth / 2) - (gameDataStart->ball[i].size / 2);
		gameDataStart->ball[i].coordinates.y = (gameAreaHeight - 50);
		gameDataStart->ball[i].speed = speed;
		gameDataStart->ball[i].isActive = FALSE;
	}
	return TRUE;
}

//GameData -> Blocks
VOID InitBlocksrray(_gameData* gameDataStart, INT size) {
	//TODO: ADD SYNC MECHANISM HERE (gameDataStart is used in main thread (to generate new maps) and in the ball thread (to destroy blocks))
	for (INT i = 0; i < size; i++) {
		gameDataStart->block[i].coordinates.x = -1;
		gameDataStart->block[i].coordinates.y = -1;
		gameDataStart->block[i].type = normal;
	}
}
BOOL BlockIsInPos(_gameData gameData, INT x, INT y, INT size) {
	for (INT i = 0; i < size; i++)
		if (gameData.block[i].coordinates.x == x && gameData.block[i].coordinates.y == y)
			return TRUE;
	return FALSE;
}
BOOL GenerateMap(UINT seed, _gameSettings* gameSettings, _gameData* gameDataStart) {
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
		posX,
		posY;

	gameSettings->totalBlocks = MIN_BLOCKS + (rand() % (MAX_BLOCKS-MIN_BLOCKS+1));	//Random value between [MIN_BLOCKS, MAX_BLOCKS]
	
	InitBlocksrray(gameDataStart, MAX_BLOCKS);	//Always intialize ALL blocks, 
	for (INT i = 0; i < gameSettings->totalBlocks; i++) {
		do {
			posX = borderPadding + ((rand() % maxBlocksPerLine) * gameSettings->blockDimensions.width);
			posY = borderPadding + ((rand() % maxBlockLines) * gameSettings->blockDimensions.height);
		} while (BlockIsInPos((*gameDataStart), posX, posY, i));	//Aqui uso "i" em vez do tamanho do array, porque nao vale a pena verificar espaços vazios do array
		gameDataStart->block[i].coordinates.x = posX;
		gameDataStart->block[i].coordinates.y = posY;
	}
	return TRUE;
}

//Game -> Initializations
BOOL InitializeSyncMechanisms() {
	hPlayerMutex = CreateMutex(
		NULL,	//Canoot be inherited by child processes
		FALSE,	//The server doesn't "own" this mutex
		NULL);	//Nameless mutex
	hGameSettingsMutex = CreateMutex(
		NULL,	//Canoot be inherited by child processes
		FALSE,	//The server doesn't "own" this mutex
		NULL);	//Nameless mutex

	return (hPlayerMutex != NULL && hGameSettingsMutex != NULL);
}
BOOL InitializeEmptyTopTen(PHKEY topTenKey) {
	//NEW REGISTRY KEY	(initizlizes all positions to -1)
	INT defaultValue = 0;
	_TCHAR name[] = _T("1");
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
BOOL LoadGameData(_gameSettings* gameSettings, _gameData* gameDataStart, INT ballSpeed, INT ballize, INT gameAreaWidth, INT gameAreaHeight) {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	LoadBalls(gameDataStart, ballSpeed, ballize, gameAreaWidth, gameAreaHeight);
	GenerateMap(time(0), gameSettings, gameDataStart);

	gameDataStart->clientMsgPos = 0;
	DWORD msgSize = sizeof(_clientMsg);
	DWORD freeSpace = sysInfo.dwAllocationGranularity - sizeof(_gameMsgNewUser) - sizeof(_serverResponse);
	gameDataStart->maxClientMsgs = (DWORD) (freeSpace / msgSize);
	return TRUE;
}
BOOL LoadClientsArray(_client* player) {
	for (INT i = 0; i < MAX_PLAYERS; i++) {
		player[i].id = -1;
		player[i].score = -1;
		memset(player[i].username, 0, USERNAME_MAX_LENGHT);
	}
	return TRUE;
}
BOOL LoadGameSettings(const _TCHAR* fileName, _gameSettings* gameSettings) {
	FILE *fp;
	if (_tfopen_s(&fp, fileName, _T("r")) == 0) {
		_tprintf(_T("Successfully opened file: %s\n"), fileName);

		_TCHAR val[10];
		_fgetts(val, 10, fp);
		gameSettings->maxSpectators = _tstoi(val);
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
BOOL InitializedServer(HANDLE* hFileMapping, LPVOID* messageBaseAddr, _gameData** gameDataStart,
					_gameMsgNewUser** gameMsgNewUser, _serverResponse** serverResponse, _clientMsg** messageStart,
					const _TCHAR* defaultsFileName, _gameSettings* gameSettings, PHKEY topTenKey,
					HANDLE* hThreadBall, HANDLE* hThreadNewUsers, _newUsersParam* param,
					_client* player, int* loggedInPlayers) {
	if (LoadSharedInfo(hFileMapping, messageBaseAddr, gameDataStart, gameMsgNewUser, serverResponse, messageStart) &&
		LoadGameSettings(defaultsFileName, gameSettings) &&
		LoadGameData(gameSettings, (*gameDataStart), gameSettings->defaultBallSpeed, gameSettings->defaultBallSize, gameSettings->dimensions.width, gameSettings->dimensions.height) &&
		LoadClientsArray(player) &&
		LoadTopTen(topTenKey) &&
		InitializeSyncMechanisms() &&
		InitThreads(hThreadBall, hThreadNewUsers, gameSettings, (*gameMsgNewUser), player, loggedInPlayers,  param))
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
VOID Start(_gameSettings* gameSettings, HANDLE hThreadBall) {
	if (ResumeThread(hThreadBall) != -1) {
		gameSettings->hasStarted = TRUE;
		_tprintf(_T("Successfully initialized thread: 'ThreadBall'\n"));
	}
	else
		_tprintf(_T("Error starting game.\nError code: 0x%x\n"), GetLastError());
}
VOID NewMap(_gameSettings* gameSettings, _gameData* gameDataStart) {
	if(GenerateMap(time(0), gameSettings, gameDataStart))
		_tprintf(_T("Successfully generated new map\n"));
}
VOID ShowTop10(PHKEY topTenKey) {
	INT data;		
	_TCHAR name[] = _T("1");
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
			return;
		}
	}
	if ((valueStatus = RegGetValueA(*topTenKey, NULL, "10", RRF_RT_REG_DWORD, &pdwType, &data, &pcbData)) == ERROR_SUCCESS)	//inicializa a 10ª posição
		_tprintf(_T(" #%d: %d\n"), 10, data);
	else
		_tprintf(_T("ERROR GETTING VALUE: 10\nError code: %ld\n"), valueStatus);
}
VOID CmdLoop(_gameSettings* gameSettings, PHKEY topTenKey, HANDLE hThreadBall, _gameData* gameDataStart) {
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
			Start(gameSettings, hThreadBall);
		else if (_tcscmp(cmd, _T("newMap")) == 0)
			NewMap(gameSettings, gameDataStart);
		else if (_tcscmp(cmd, _T("top10")) == 0)
			ShowTop10(topTenKey);
	} while (_tcscmp(cmd, _T("exit")) != 0);
}

//Dynamic Memory Management
VOID CleanUp(LPVOID messageBaseAddr, HANDLE hFileMapping, _gameData* gameDataStart) {
	CloseSharedInfoHandles(messageBaseAddr, hFileMapping, gameDataStart);
}

//Main
INT _tmain(INT argc, const _TCHAR* argv[]) {
	if (argc == 2) {
		//Game vars
		_gameSettings gameSettings;
		_client player[MAX_PLAYERS];
		int loggedInPlayers = 0;	//This var exists to avoid looping the player's array everytime to count
		INT spectators = 0;

		//File mapping vars
		HANDLE hFileMapping;

		LPVOID messageBaseAddr;
		_gameData* gameDataStart;
		_gameMsgNewUser* gameMsgNewUser;
		_serverResponse* serverResp;
		_clientMsg* messageStart;

		//Thread handles
		HANDLE hThreadBall, hThreadNewUsers;

		//Thread params
		_newUsersParam param;

		//Registry key vars
		HKEY topTenKey;

		_tprintf(_T("Arkanoid server:\nExecutable location: %s\n-------------------------------------------\nInitializing...\n"), argv[0]);

		if (InitializedServer(&hFileMapping, &messageBaseAddr, &gameDataStart,
							  &gameMsgNewUser, &serverResp, &messageStart,
							  argv[1], &gameSettings, &topTenKey,
							  &hThreadBall, &hThreadNewUsers, &param,
							  player, &loggedInPlayers)) {
			_tprintf(_T("Done!\n"));
			CmdLoop(&gameSettings, &topTenKey, hThreadBall, gameDataStart);
			CleanUp(messageBaseAddr, hFileMapping, gameDataStart);
		}
		else {
			_tprintf(_T("-------------------------------------------\nPress ENTER to exit."));
			getchar();
		}
	}
	return 0;
}