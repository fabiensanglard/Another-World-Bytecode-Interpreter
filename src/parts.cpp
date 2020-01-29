#include "parts.h"


/*
#define MEMLIST_PART_PALETTE 0
#define MEMLIST_PART_CODE    1
#define MEMLIST_PART_VIDEO1  2
#define MEMLIST_PART_VIDEO2  3
*/

/*
	MEMLIST_PART_VIDEO1 and MEMLIST_PART_VIDEO2 are used to store polygons. 

	It seems that:
	- MEMLIST_PART_VIDEO1 contains the cinematic polygons.
	- MEMLIST_PART_VIDEO2 contains the polygons for player and enemies animations.

	That would make sense since protection screen and cinematic game parts do not load MEMLIST_PART_VIDEO2.

*/
const uint16_t memListParts[GAME_NUM_PARTS][4] = {

//MEMLIST_PART_PALETTE   MEMLIST_PART_CODE   MEMLIST_PART_VIDEO1   MEMLIST_PART_VIDEO2
	{ 0x14,                    0x15,                0x16,                0x00 }, // protection screens
	{ 0x17,                    0x18,                0x19,                0x00 }, // introduction cinematic
	{ 0x1A,                    0x1B,                0x1C,                0x11 },
	{ 0x1D,                    0x1E,                0x1F,                0x11 },
	{ 0x20,                    0x21,                0x22,                0x11 },
	{ 0x23,                    0x24,                0x25,                0x00 }, // battlechar cinematic
	{ 0x26,                    0x27,                0x28,                0x11 },
	{ 0x29,                    0x2A,                0x2B,                0x11 },
	{ 0x7D,                    0x7E,                0x7F,                0x00 },
	{ 0x7D,                    0x7E,                0x7F,                0x00 }  // password screen
};
