#pragma once
//Server Constants------------------------
#define CMD_SIZE 128

//GAME SETTINGS---------------------------
struct gameSettings_STRUCT {
	int maxPlayers,
		levels,
		speedUps,
		slowDowns,
		duration,
		speedUpChance,
		slowDownChance,
		lives;
} typedef _gameSettings;

//COORDINATES-----------------------------
struct coordinates_STRUCT {
	int x,
		y;
} typedef _coordinates;

//GAME AREA-------------------------------
struct gameArea_STRUCT {
	int width,
		height;
} typedef _gameArea;

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
	int speed;
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
	int size;
} typedef _block;

//PERK------------------------------------
enum perkType_ENUM {
	speedUp,
	slowDown,
	extraLife,
	triple
} typedef _perkType;
struct perk_STRUCT {
	_perkType type;
	_coordinates coordinates;
	int size,
		speed;
} typedef _perk;