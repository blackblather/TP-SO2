#pragma once
//Server Constants------------------------
#define CMD_SIZE 128
#define MAPPED_FILE_NAME _T("../Server/SharedInfo.txt")
#define USERNAME_MAX_LENGHT 64	//64 characters, including '\0'
#define MIN_BLOCKS 30	//Arbitrary value (used in map generation)
#define MAX_BLOCKS 50	//Arbitrary value

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
	INT maxPlayers,
		maxSpectators,
		maxBalls,
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
	INT size,
		speed;
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
	INT id, score;
	TCHAR username[USERNAME_MAX_LENGHT];
} typedef _client;

//GAMEDATA--------------------------------
struct gameData_STRUCT {
	_block block[MAX_BLOCKS];
} typedef _gameData;

//GAMEMSG - NEW USER----------------------
struct gameMsgNewUser_STRUCT {
	TCHAR username[256];
	BOOL response;
} typedef _gameMsgNewUser;