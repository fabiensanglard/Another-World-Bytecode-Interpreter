#ifndef __AW_PARTS_
#define __AW_PARTS_

#include "intern.h"
#include "endian.h"

//The game is divided in 10 parts.
#define GAME_NUM_PARTS 10

#define GAME_PART_FIRST  0x3E80
#define GAME_PART1       0x3E80
#define GAME_PART2       0x3E81   //Introductino
#define GAME_PART3       0x3E82
#define GAME_PART4       0x3E83   //Wake up in the suspended jail
#define GAME_PART5       0x3E84
#define GAME_PART6       0x3E85   //BattleChar sequence
#define GAME_PART7       0x3E86
#define GAME_PART8       0x3E87
#define GAME_PART9       0x3E88
#define GAME_PART10      0x3E89
#define GAME_PART_LAST   0x3E89

extern const uint16 memListParts[GAME_NUM_PARTS][4];

//For each part of the game, four resources are referenced.
#define MEMLIST_PART_PALETTE 0
#define MEMLIST_PART_CODE    1
#define MEMLIST_PART_VIDEO1  2
#define MEMLIST_PART_VIDEO2  3


#define MEMLIST_PART_NONE 0x00


#endif