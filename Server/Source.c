#include <stdio.h>
#include <time.h> 
#include <tchar.h>
#include <Windows.h> 
#include "Resources.h"
 
//Server specific structs
struct newUsersParam_STRUCT {
	_gameSettings* gameSettings;
	_gameMsgNewUser* gameMsgNewUser;
	_base* baseBaseAddr;
	_client* player;
	INT* loggedInPlayers;
} typedef _newUsersParam;
struct processPlayerMsgParam_STRUCT {
	INT maxClientMsgs;
	INT* loggedInPlayers;
	_gameSettings* gameSettings;
	_client* player;
	_clientMsg* clientMsg;
} typedef _processPlayerMsgParam;

//Client vars
HANDLE hPlayerMutex;		//To use "player" array and/or "loggedInPlayers", use this mutex
//Game Settings Mutex
HANDLE hGameSettingsMutex;
//Game Data Mutex
HANDLE hGameDataMutex;

//File Mapping
LPVOID LoadFileView(HANDLE hFileMapping, int offset, int size) {
	return MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ | FILE_MAP_WRITE,
		0,
		offset,
		size);
}
BOOL LoadSharedInfo(HANDLE* hFileMapping, LPVOID* messageBaseAddr, _gameData** gameDataStart, _gameMsgNewUser** gameMsgNewUser, _clientMsg** messageStart) {
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
			(*gameMsgNewUser) = (_gameMsgNewUser*)(*messageBaseAddr);
			(*messageStart) = (*gameMsgNewUser) + 1;
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

//Used in "Threads -> Player Msgs" and "Threads -> New Users"
void NotifyPlayers(_client* player, INT loggedInPlayers) {
	for (INT i = 0; i < loggedInPlayers; i++)
		SetEvent(player[i].hUpdateBaseEvent);
}

//Threads -> Ball
DWORD WINAPI ThreadBall(LPVOID lpParameter) {
	//Wait for other threads to be done using gameData
	WaitForSingleObject(hGameDataMutex, INFINITE);
		
	ReleaseMutex(hGameDataMutex);
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
BOOL AddUserToLoggedInUsersArray(TCHAR updateMapEventName[20], TCHAR username[USERNAME_MAX_LENGHT], _client* player, INT pos, _base* base) {
	player[pos].hUpdateBaseEvent = CreateEvent(
		NULL,	//Security attributes
		FALSE,	//Automatic reset
		FALSE,	//Inital state = not set
		updateMapEventName);	//Event name (clients need the name to wait for their event to be set)
	if (player[pos].hUpdateBaseEvent != NULL) {
		player[pos].base = base + pos;
		player[pos].base->hasPlayer = TRUE;
		_tcscpy_s(player[pos].username, USERNAME_MAX_LENGHT, username);
		return TRUE;
	}
	return FALSE;
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
	if (hNewUserMutex != NULL && hNewUserServerEvent != NULL && hNewUserClientEvent != NULL) {
		while (1) {
			//Wait for a client to tell the server to process his username
			WaitForSingleObject(hNewUserServerEvent, INFINITE);
			//Wait for other threads to be done using player's array and loggedInPlayers var
			WaitForSingleObject(hPlayerMutex, INFINITE);
			//Wait for other threads to be done using gameSettings struct
			//I'm avoiding using WaitForMultipleObjects() to avoid creating an array of mutexes
			WaitForSingleObject(hGameSettingsMutex, INFINITE);
			if (_tcsnlen(param->gameMsgNewUser->username, USERNAME_MAX_LENGHT) > 0 && UsernameIsUnique(param->gameMsgNewUser->username, param->player) && (*param->loggedInPlayers) < MAX_PLAYERS) {
				if (!param->gameSettings->hasStarted) {
					//15 for "updateMapEvent_"
					//4 for clientId -> [0, 9999]
					//1 for '\0'
					TCHAR updateMapEventName[20];
					_stprintf_s(updateMapEventName, 20, TEXT("updateMapEvent_%d"), (*param->loggedInPlayers));
					if (AddUserToLoggedInUsersArray(updateMapEventName, param->gameMsgNewUser->username, param->player, (*param->loggedInPlayers), param->baseBaseAddr)){
						param->gameMsgNewUser->clientId = (*param->loggedInPlayers);
						_tcscpy_s(param->gameMsgNewUser->updateMapEventName, 20, updateMapEventName);
						param->gameMsgNewUser->clientAreaWidth = param->gameSettings->dimensions.width;
						param->gameMsgNewUser->clientAreaHeight = param->gameSettings->dimensions.height;
						(*param->loggedInPlayers)++;
						param->gameMsgNewUser->loggedIn = TRUE;
					}
				}
				else {
					//AddUserToSpectatorsArray();
				}
			}
			else
				param->gameMsgNewUser->loggedIn = FALSE;
			ReleaseMutex(hGameSettingsMutex);
			ReleaseMutex(hPlayerMutex);
			SetEvent(hNewUserClientEvent);
		}
	}
	return 1;
}

//Threads -> Player Msgs
BOOL IsValidPlayerMsg(_clientMsg msg, _client* player, INT gameAreaWidth) {
	//TODO: Take into account other "player bars" to avoid collisions here
	if (msg.move == moveLeft && player[msg.clientId].base->rectangle.left - player[msg.clientId].base->speed >= 0)
		return TRUE;
	if (msg.move == moveRight && player[msg.clientId].base->rectangle.right + player[msg.clientId].base->speed <= gameAreaWidth)
		return TRUE;
	return FALSE;
}
void UpdatePlayerBasePos(_clientMsg msg, _client* player) {
	switch (msg.move) {
		case moveLeft: {
			player[msg.clientId].base->rectangle.left -= player[msg.clientId].base->speed;
			player[msg.clientId].base->rectangle.right -= player[msg.clientId].base->speed;
		}break;
		case moveRight: {
			player[msg.clientId].base->rectangle.left += player[msg.clientId].base->speed;
			player[msg.clientId].base->rectangle.right += player[msg.clientId].base->speed;
		} break;
	}
}
void WipeClientMsg(_clientMsg* clientMsg) {
	clientMsg->move = none;
}
BOOL ServerMsgPosReachedTheEnd(INT pos, INT max) {
	return (pos == (max - 1));
}
DWORD WINAPI ThreadProcessPlayerMsg(LPVOID lpParameter) {
	_processPlayerMsgParam* param = (_processPlayerMsgParam*)lpParameter;
	INT serverMsgPos = 0;	//Index used by the server when reading messages
	HANDLE hNewPlayerMsgMutex = CreateMutex(
		NULL,		//Canoot be inherited by child processes
		FALSE,		//The server doesn't "own" this mutex
		TEXT("newPlayerMsgMutex"));	//Mutex name
	HANDLE hNewPlayerMsgSemaphore = CreateSemaphore(
		NULL,		//Canoot be inherited by child processes
		0,			//Initial count
		param->maxClientMsgs,
		TEXT("newPlayerMsgSemaphore"));	//Semaphore name
	if(hNewPlayerMsgMutex != NULL && hNewPlayerMsgSemaphore != NULL){
		while (1) {
			WaitForSingleObject(hNewPlayerMsgSemaphore, INFINITE);
				//Wait for other threads to be done using player's array
				WaitForSingleObject(hPlayerMutex, INFINITE);
					if (ServerMsgPosReachedTheEnd(serverMsgPos, param->maxClientMsgs))
						serverMsgPos = 0;
					//Game has started and player msg is valid
					if (param->gameSettings->hasStarted && IsValidPlayerMsg(param->clientMsg[serverMsgPos], param->player, param->gameSettings->dimensions.width)) {
						//Wait for other threads to be done using gameData
						WaitForSingleObject(hGameDataMutex, INFINITE);
							//Update gameData->base
							UpdatePlayerBasePos(param->clientMsg[serverMsgPos], param->player);
							//Notify and await confirmation
							NotifyPlayers(param->player, (*param->loggedInPlayers));
						ReleaseMutex(hGameDataMutex);
					}
					WipeClientMsg(param->clientMsg + serverMsgPos);	//All param->clientMsg->move are initialized to 0 (none)
					serverMsgPos++;
				ReleaseMutex(hPlayerMutex);
		}
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
BOOL InitThreadNewUsers(HANDLE* hThreadNewUsers, _gameSettings* gameSettings, _gameMsgNewUser* gameMsgNewUser, _client* player,  int* loggedInPlayers, _base* baseBaseAddr, _newUsersParam* param) {
	param->gameSettings = gameSettings;
	param->gameMsgNewUser = gameMsgNewUser;
	param->baseBaseAddr = baseBaseAddr;
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
BOOL InitThreadProcessPlayerMsg(HANDLE* hThreadProcessPlayerMsg, INT maxClientMsgs, INT* loggedInPlayers, _gameSettings* gameSettings, _client* player, _clientMsg* clientMsg, _processPlayerMsgParam* param) {
	param->maxClientMsgs = maxClientMsgs;
	param->loggedInPlayers = loggedInPlayers;
	param->gameSettings = gameSettings;
	param->player = player;
	param->clientMsg = clientMsg;
	(*hThreadProcessPlayerMsg) = CreateThread(
		NULL,					//hThreadProcessPlayerMsg cannot be inherited by child processes
		0,						//Default stack size
		ThreadProcessPlayerMsg,	//Function to execute
		param,					//Function param
		0,						//The thread runs immediately after creation
		NULL					//Thread ID is not stored anywhere
	);
	if ((*hThreadProcessPlayerMsg) != NULL)
		return TRUE;
	_tprintf(_T("ERROR CREATING 'process player message' THREAD\n"));
	return FALSE;
}
BOOL InitThreads(HANDLE* hThreadBall, HANDLE* hThreadNewUsers, HANDLE* hThreadProcessPlayerMsg,
				 _gameSettings* gameSettings, _gameMsgNewUser* gameMsgNewUser, _client* player,
				 INT* loggedInPlayers, _newUsersParam* newUsersParam, INT maxClientMsgs,
				 _clientMsg* clientMsg, _processPlayerMsgParam* processPlayerMsgParam, _base* baseBaseAddr) {
	if (InitThreadBall(hThreadBall)) {
		_tprintf(_T("Initialized 'ball' thread [SUSPENDED]\n"));
		if (InitThreadNewUsers(hThreadNewUsers, gameSettings, gameMsgNewUser, player, loggedInPlayers, baseBaseAddr, newUsersParam)) {
			_tprintf(_T("Initialized 'new users' thread [RUNNING]\n"));
			if(InitThreadProcessPlayerMsg(hThreadProcessPlayerMsg, maxClientMsgs, loggedInPlayers, gameSettings, player, clientMsg, processPlayerMsgParam)){
				_tprintf(_T("Initialized 'process player message' thread [RUNNING]\n"));
				return TRUE;
			}
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
VOID LoadBalls(_gameData* gameDataStart, INT speed, INT size, INT gameAreaWidth, INT gameAreaHeight) {
	for (INT i = 0; i < MAX_BALLS; i++) {
		gameDataStart->ball[i].direction = topRight;
		gameDataStart->ball[i].rectangle.left = (gameAreaWidth / 2) - (size / 2);
		gameDataStart->ball[i].rectangle.top = (gameAreaHeight - 100);
		gameDataStart->ball[i].rectangle.right = gameDataStart->ball[i].rectangle.left + size;
		gameDataStart->ball[i].rectangle.bottom = gameDataStart->ball[i].rectangle.top + size;
		gameDataStart->ball[i].speed = speed;
		gameDataStart->ball[i].isActive = FALSE;
	}
}

//GameData -> Blocks
VOID InitBlocks(_gameData* gameDataStart, INT size) {
	//TODO: ADD SYNC MECHANISM HERE (gameDataStart is used in main thread (to generate new maps) and in the ball thread (to destroy blocks))
	for (INT i = 0; i < size; i++) {
		gameDataStart->block[i].type = normal;
		gameDataStart->block[i].rectangle.bottom = 0;
		gameDataStart->block[i].rectangle.top = 0;
		gameDataStart->block[i].rectangle.left = 0;
		gameDataStart->block[i].rectangle.right = 0;
		gameDataStart->block[i].destroyed = TRUE;
	}
}
BOOL BlockIsInPos(_gameData gameData, INT x, INT y, INT size) {
	for (INT i = 0; i < size; i++)
		if (gameData.block[i].rectangle.left == x && gameData.block[i].rectangle.right == y)
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
	
	InitBlocks(gameDataStart, MAX_BLOCKS);	//Always intialize ALL blocks, 
	for (INT i = 0; i < gameSettings->totalBlocks; i++) {
		do {
			posX = borderPadding + ((rand() % maxBlocksPerLine) * gameSettings->blockDimensions.width);
			posY = borderPadding + ((rand() % maxBlockLines) * gameSettings->blockDimensions.height);
		} while (BlockIsInPos((*gameDataStart), posX, posY, i));	//Aqui uso "i" em vez do tamanho do array, porque nao vale a pena verificar espaços vazios do array
		gameDataStart->block[i].rectangle.left = posX;
		gameDataStart->block[i].rectangle.top = posY;
		gameDataStart->block[i].rectangle.right = posX + gameSettings->blockDimensions.width;
		gameDataStart->block[i].rectangle.bottom = posY + gameSettings->blockDimensions.height;
		gameDataStart->block[i].destroyed = FALSE;
	}
	return TRUE;
}

//GameData -> Bases
VOID LoadBases(_gameData* gameDataStart, INT gameAreaWidth, INT gameAreaHeight) {
	//size: 70
	//speed: 2
	for (INT i = 0; i < MAX_PLAYERS; i++) {
		gameDataStart->base[i].rectangle.left = (gameAreaWidth / 2) - (70 / 2);
		gameDataStart->base[i].rectangle.top = (gameAreaHeight - 80);
		gameDataStart->base[i].rectangle.right = gameDataStart->base[i].rectangle.left + 70;
		gameDataStart->base[i].rectangle.bottom = gameDataStart->base[i].rectangle.top + 20;
		gameDataStart->base[i].hasPlayer = FALSE;
		gameDataStart->base[i].speed = 2;
	}
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
	hGameDataMutex = CreateMutex(
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
	LoadBases(gameDataStart, gameAreaWidth, gameAreaHeight);
	GenerateMap(time(0), gameSettings, gameDataStart);

	gameDataStart->clientMsgPos = 0;
	DWORD msgSize = sizeof(_clientMsg);
	DWORD freeSpace = sysInfo.dwAllocationGranularity - sizeof(_gameMsgNewUser);
	gameDataStart->maxClientMsgs = (DWORD) (freeSpace / msgSize);
	return TRUE;
}
BOOL LoadClientsArray(_client* player) {
	for (INT i = 0; i < MAX_PLAYERS; i++) {
		player[i].score = -1;
		player[i].base = NULL;
		memset(player[i].username, 0, USERNAME_MAX_LENGHT);
		player[i].hUpdateBaseEvent = NULL;
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
					_gameMsgNewUser** gameMsgNewUser, _clientMsg** messageStart,
					const _TCHAR* defaultsFileName, _gameSettings* gameSettings, PHKEY topTenKey,
					HANDLE* hThreadBall, HANDLE* hThreadNewUsers, _newUsersParam* newUsersParam,
					_client* player, int* loggedInPlayers, HANDLE* hThreadProcessPlayerMsg, _processPlayerMsgParam* processPlayerMsgParam) {
	if (LoadSharedInfo(hFileMapping, messageBaseAddr, gameDataStart, gameMsgNewUser, messageStart) &&
		LoadGameSettings(defaultsFileName, gameSettings) &&
		LoadGameData(gameSettings, (*gameDataStart), gameSettings->defaultBallSpeed, gameSettings->defaultBallSize, gameSettings->dimensions.width, gameSettings->dimensions.height) &&
		LoadClientsArray(player) &&
		LoadTopTen(topTenKey) &&
		InitializeSyncMechanisms() &&
		InitThreads(hThreadBall, hThreadNewUsers, hThreadProcessPlayerMsg, gameSettings, (*gameMsgNewUser), player, loggedInPlayers, newUsersParam, (*gameDataStart)->maxClientMsgs, (*messageStart), processPlayerMsgParam, (*gameDataStart)->base))
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
		_clientMsg* messageStart;

		//Thread handles
		HANDLE hThreadBall, hThreadNewUsers, hThreadProcessPlayerMsg;

		//Thread params
		_newUsersParam param;
		_processPlayerMsgParam processPlayerMsgParam;

		//Registry key vars
		HKEY topTenKey;

		_tprintf(_T("Arkanoid server:\nExecutable location: %s\n-------------------------------------------\nInitializing...\n"), argv[0]);

		if (InitializedServer(&hFileMapping, &messageBaseAddr, &gameDataStart,
							  &gameMsgNewUser, &messageStart,
							  argv[1], &gameSettings, &topTenKey,
							  &hThreadBall, &hThreadNewUsers, &param,
							  player, &loggedInPlayers, &hThreadProcessPlayerMsg, &processPlayerMsgParam)) {
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