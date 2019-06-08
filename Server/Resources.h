#pragma once
//Server Constants------------------------
#define CMD_SIZE 128
#define MAPPED_FILE_NAME _T("../Server/SharedInfo.txt")
#define USERNAME_MAX_LENGHT 256	//64 characters, including '\0'
#define MIN_BLOCKS 30	//Arbitrary value (used in map generation)
#define MAX_BLOCKS 50	//Arbitrary value (used in map generation)
#define MAX_BALLS 3
#define MAX_PLAYERS 3

//RECTANGLE-------------------------------
struct rect_STRUCT {
	INT width,
		height;
} typedef _rect;

//COORDINATES-----------------------------
struct coordinates_STRUCT {
	INT x,
		y;
} typedef _coordinates;

//GAME SETTINGS---------------------------
struct gameSettings_STRUCT {
	//Configuraveis em "Defaults.txt"
	/*NOTA: MAX_BALLS e MAX_PLAYERS foram retirados do ficheiro de configuração,
	  para evitar ter que criar uma versão da função "malloc" que reservasse
	  espaço, de forma dinâmica, DENTRO da fileview, sem bugs. Desta maneira, simplifico
	  o código, sacrificando um pouco de espaço pois reservo sempre 2 arrays de tam
	  MAX_BALLS e MAX_PLAYERS*/
	INT maxSpectators,
		maxPerks,
		defaultBallSpeed,
		defaultBallSize,
		levels,
		speedUps,
		slowDowns,
		duration,
		speedUpChance,
		slowDownChance,
		lives,
		totalBlocks;
	_rect dimensions,
		blockDimensions;

	//Não configuráveis em "Defaults.txt"
	BOOL hasStarted;	//Default sempre 0
} typedef _gameSettings;

//BALL------------------------------------
enum ballDirection_ENUM {
	topRight,
	topLeft,
	bottomRight,
	bottomLeft
} typedef _ballDirection;
struct ball_STRUCT {
	_ballDirection direction;
	_coordinates coordinates;
	INT size,
		speed;
	BOOL isActive;
} typedef _ball;

//BASE------------------------------------
struct base_STRUCT {
	_coordinates coordinates;
	INT speed;
	_rect dimensions;
} typedef _base;

//BLOCK-----------------------------------
enum blockType_ENUM {
	normal,
	strong,
	magic
} typedef _blockType;
struct block_STRUCT {
	_blockType type;
	_coordinates coordinates;
} typedef _block;

//PERK------------------------------------
enum perkType_ENUM {
	speedUp,
	slowDown,
	extraLife,
	extraBall,
	triple
} typedef _perkType;
struct perk_STRUCT {
	_perkType type;
	_coordinates coordinates;
	INT size,
		speed;
} typedef _perk;

//CLIENT----------------------------------
struct client_STRUCT {
	INT score;
	_base* base;
	TCHAR username[USERNAME_MAX_LENGHT];
	HANDLE hUpdateMapEvent;
} typedef _client;

//GAMEDATA--------------------------------
struct gameData_STRUCT {
	_block block[MAX_BLOCKS];	//Blocks array
	_base base[MAX_PLAYERS];	//Base(s) array
	_ball ball[MAX_BALLS];	//Ball(s) array
	INT clientMsgPos;	//Index used by clients when writing messages
	DWORD maxClientMsgs;	//Depende do tamanho das mensagens
} typedef _gameData;

//GAMEMSG - NEW USER----------------------
struct gameMsgNewUser_STRUCT {
	TCHAR updateMapEventName[20];
	TCHAR username[256];
	BOOL loggedIn;
	INT clientId;
} typedef _gameMsgNewUser;

//GAMEMSG - CLIENT MSG--------------------
enum movement_ENUM {
	none,
	moveLeft,
	moveRight
} typedef _movement;
struct clientMsg_STRUCT {
	INT clientId;
	_movement move;
} typedef _clientMsg;