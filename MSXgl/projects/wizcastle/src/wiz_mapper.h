// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  MSX-DOS 2 Memory Mapper & Asset Bank Manager (WIZDATA.DAT)
//─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "msxgl.h"
#include "dos_mapper.h"

//=============================================================================
// DEFINES & ENUMS
//=============================================================================

// Logical segment indices inside g_WizSegs[] (7 segments x 16 KB = 112 KB)
#define WIZ_SEG_SPR32_0         0   // 32x32 Sprites 0..15  (16 x 1024 B = 16 KB)
#define WIZ_SEG_SPR32_1         1   // 32x32 Sprites 16..31 (16 x 1024 B = 16 KB)
#define WIZ_SEG_SPR32_2         2   // 32x32 Sprites 32..39 (8 x 1024 B) + "WIZDATA1" tag at 0x3F00
#define WIZ_SEG_MAP_TILES       3   // 40 16x16 Map Tiles (10 KB) + 10 16x16 Treasure Icons at 0x2800
#define WIZ_SEG_YJK_COMBAT      4   // 160x100 SCREEN 12 YJK Splash: Combat / Title (16,000 B)
#define WIZ_SEG_YJK_VICTORY     5   // 160x100 SCREEN 12 YJK Splash: Victory        (16,000 B)
#define WIZ_SEG_YJK_DEATH       6   // 160x100 SCREEN 12 YJK Splash: Death          (16,000 B)
#define WIZ_NUM_MAPPER_SEGS     7

// Canonical 40 Object IDs (matching winwiz/wfuncs.cpp object_data[40])
enum WizObjectId
{
	OBJ_UNSEEN_ROOM     = 0,
	OBJ_EMPTY_ROOM      = 1,
	OBJ_CASTLE_ENTRANCE = 2,
	OBJ_STAIRS_UP       = 3,
	OBJ_STAIRS_DOWN     = 4,
	OBJ_POOL            = 5,
	OBJ_CHEST           = 6,
	OBJ_GOLD_PIECES     = 7,
	OBJ_FLARES          = 8,
	OBJ_WARP            = 9,
	OBJ_SINKHOLE        = 10,
	OBJ_CRYSTAL_ORB     = 11,
	OBJ_BOOK            = 12,
	OBJ_KOBOLD          = 13,
	OBJ_ORC             = 14,
	OBJ_KRESH           = 15,
	OBJ_GOBLIN          = 16,
	OBJ_OGRE            = 17,
	OBJ_TROLL           = 18,
	OBJ_MINOTAUR        = 19,
	OBJ_UR_VILE         = 20,
	OBJ_GARGOYLE        = 21,
	OBJ_CHIMERA         = 22,
	OBJ_BALROG          = 23,
	OBJ_DRAGON          = 24,
	OBJ_VENDOR          = 25,
	OBJ_RUBY_RED        = 26,
	OBJ_NORN_STONE      = 27,
	OBJ_PALE_PEARL      = 28,
	OBJ_OPAL_EYE        = 29,
	OBJ_GREEN_GEM       = 30,
	OBJ_BLUE_FLAME      = 31,
	OBJ_PALANTIR        = 32,
	OBJ_SILMARIL        = 33,
	OBJ_RUNESTAFF       = 34,
	OBJ_ORB_OF_ZOT      = 35,
	OBJ_HUMAN           = 36,
	OBJ_DWARF           = 37,
	OBJ_HOBBIT          = 38,
	OBJ_ELF             = 39,
	OBJ_COUNT           = 40
};

//=============================================================================
// FUNCTIONS
//=============================================================================

// Returns TRUE if WIZDATA.DAT has been allocated and loaded into Mapper RAM
bool Wiz_MapperIsReady(void);

// Returns the number of allocated 16 KB segments
u8 Wiz_MapperGetAllocatedCount(void);

// Allocate 7 Memory Mapper segments (112 KB) and stream WIZDATA.DAT from disk
// while rendering a graphical progress bar in SCREEN 7.
bool Wiz_MapperInitAndLoad(void);

// Free all allocated Memory Mapper segments and restore default TPA Page 2
void Wiz_MapperCleanup(void);

// Map one of our 7 logical segments (WIZ_SEG_*) into Z80 Page 2 (0x8000-0xBFFF)
void Wiz_MapperMapSeg(u8 logicalSeg);

// Restore the original MSX-DOS 2 TPA segment into Z80 Page 2 (0x8000-0xBFFF)
void Wiz_MapperRestorePage2(void);

// Stream a 160x100 SCREEN 12 YJK panel (WIZ_SEG_YJK_*) from Mapper to VRAM Page 0
// Note: destX must be a multiple of 4 (YJK 4-pixel packet alignment)
void Wiz_MapperDrawYJKPanel(u8 yjkSeg, u8 destX, u8 destY);

// Upload all 40 16x16 Map Tiles and 10 16x16 Treasure Icons from Segment 3
// into SCREEN 8 VRAM Page 1 (off-screen cache at Y=256..319) for fast VDP HMMM blits
void Wiz_MapperUploadTilesToVRAMPage1(void);

// Hardware-blit a 16x16 Map Tile (objId 0..39) from VRAM Page 1 cache to Page 0 (destX, destY)
void Wiz_MapperBlitTile16(u8 objId, u8 destX, u8 destY);

// Hardware-blit a 16x16 Treasure Icon (treasIdx 0..9) from VRAM Page 1 cache to Page 0 (destX, destY)
void Wiz_MapperBlitTreasureIcon16(u8 treasIdx, u8 destX, u8 destY);

// Draw a 32x32 Sprite (objId 0..39) from Mapper Segments 0..2 into SCREEN 8 Page 0 (destX, destY)
void Wiz_MapperDrawSprite32(u8 objId, u8 destX, u8 destY);
