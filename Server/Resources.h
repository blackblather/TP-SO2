#pragma once
//Server Constants------------------------
#define CMD_SIZE 128
#define MAPPED_FILE_NAME _T("../Server/SharedInfo.txt")
//RECTANGLE-------------------------------
struct rect_STRUCT {
	int width,
		height;
} typedef _rect;

//COORDINATES-----------------------------
struct coordinates_STRUCT {
	int x,
		y;
} typedef _coordinates;

//GAME SETTINGS---------------------------
struct gameSettings_STRUCT {
	int maxPlayers,
		maxBalls,
		defaultBallSpeed,
		defaultBallSize,
		levels,
		speedUps,
		slowDowns,
		duration,
		speedUpChance,
		slowDownChance,
		lives;
	_rect dimensions;
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
	int size,
		speed;
	BOOL isActive;
} typedef _ball;

//BASE------------------------------------
struct base_STRUCT {
	_coordinates coordinates;
	int size,
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
	_rect dimensions;
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
	int size,
		speed;
} typedef _perk;