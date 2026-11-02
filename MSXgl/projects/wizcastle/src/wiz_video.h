// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Video Engine Header (SCREEN 8 Main UI & SCREEN 12 Event Screens)
//─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "msxgl.h"
#include "wiz_mapper.h"

//=============================================================================
// SCREEN 8 GRB (3-3-2: GGGRRRBB) COLOR PALETTE CONSTANTS
//=============================================================================

#define WIZ_COL_BLACK           0x00    // G:0 R:0 B:0 - Pure Black
#define WIZ_COL_WHITE           0xFF    // G:7 R:7 B:3 - Pure White
#define WIZ_COL_GRAY_DARK       0x24    // G:1 R:1 B:0 - Dark Stone Gray
#define WIZ_COL_GRAY_MID        0x49    // G:2 R:2 B:1 - Medium Slate Gray
#define WIZ_COL_GRAY_LIGHT      0x92    // G:4 R:4 B:2 - Light Silver Gray

#define WIZ_COL_NAVY            0x02    // G:0 R:0 B:2 - Deep Royal Blue
#define WIZ_COL_BLUE_MID        0x27    // G:1 R:1 B:3 - Slate Blue
#define WIZ_COL_CYAN            0xE3    // G:7 R:0 B:3 - Bright Cyan

#define WIZ_COL_GOLD            0xFC    // G:7 R:7 B:0 - Bright Gold / Yellow
#define WIZ_COL_GOLD_DARK       0x90    // G:4 R:4 B:0 - Dark Bronze / Ochre
#define WIZ_COL_ORANGE          0x7C    // G:3 R:7 B:0 - Amber Orange

#define WIZ_COL_RED_DARK        0x0C    // G:0 R:3 B:0 - Dark Crimson
#define WIZ_COL_RED             0x1C    // G:0 R:7 B:0 - Bright Red
#define WIZ_COL_RED_LIGHT       0x5D    // G:2 R:7 B:1 - Light Coral Red

#define WIZ_COL_GREEN_DARK      0x40    // G:2 R:0 B:0 - Dark Forest Green
#define WIZ_COL_GREEN           0xE0    // G:7 R:0 B:0 - Bright Emerald Green
#define WIZ_COL_MAGENTA         0x1F    // G:0 R:7 B:3 - Bright Magenta / Purple

//=============================================================================
// SCREEN 8 UI REGION COORDINATES (256x212)
//=============================================================================

// 1. Top Title & Location Banner (256x11)
#define WIZ_UI_TOPBAR_X         0
#define WIZ_UI_TOPBAR_Y         0
#define WIZ_UI_TOPBAR_W         256
#define WIZ_UI_TOPBAR_H         11

// 2. 8x8 Dungeon Floor Map Frame (Interior 128x128 at X=4..131, Y=14..141)
#define WIZ_UI_MAP_BOX_X1       2
#define WIZ_UI_MAP_BOX_Y1       12
#define WIZ_UI_MAP_BOX_X2       133
#define WIZ_UI_MAP_BOX_Y2       143
#define WIZ_UI_MAP_X            4
#define WIZ_UI_MAP_Y            14

// 3. 48x48 Room & Combat Viewport Frame (Interior 48x48 at X=136..183, Y=14..61)
#define WIZ_UI_VIEW_BOX_X1      135
#define WIZ_UI_VIEW_BOX_Y1      12
#define WIZ_UI_VIEW_BOX_X2      184
#define WIZ_UI_VIEW_BOX_Y2      62
#define WIZ_UI_VIEW_X           136
#define WIZ_UI_VIEW_Y           14
#define WIZ_UI_VIEW_SPR_X       144
#define WIZ_UI_VIEW_SPR_Y       22

// 3b. Viewport Label / Monster HP Bar Strip (X=135..184, Y=64..76)
#define WIZ_UI_VIEW_LBL_X1      135
#define WIZ_UI_VIEW_LBL_Y1      64
#define WIZ_UI_VIEW_LBL_X2      184
#define WIZ_UI_VIEW_LBL_Y2      76

// 4. Character Status HUD Panel (X=186..255, Y=12..76)
#define WIZ_UI_HUD_BOX_X1       186
#define WIZ_UI_HUD_BOX_Y1       12
#define WIZ_UI_HUD_BOX_X2       255
#define WIZ_UI_HUD_BOX_Y2       76

// 5. 10-Treasure Tray Panel (X=135..255, Y=78..143)
#define WIZ_UI_TREAS_BOX_X1     135
#define WIZ_UI_TREAS_BOX_Y1     78
#define WIZ_UI_TREAS_BOX_X2     255
#define WIZ_UI_TREAS_BOX_Y2     143

// 6. 8-Line Color Message Terminal (X=0..255, Y=145..211)
#define WIZ_UI_TERM_BOX_X1      0
#define WIZ_UI_TERM_BOX_Y1      145
#define WIZ_UI_TERM_BOX_X2      255
#define WIZ_UI_TERM_BOX_Y2      211

//=============================================================================
// SCREEN 12 (19,268 YJK COLORS) EVENT SCREEN TYPES
//=============================================================================

enum WizScreen12Type
{
	WIZ_SC12_SPLASH  = 0,   // Title / Legend of Zot (WIZ_SEG_YJK_COMBAT)
	WIZ_SC12_VICTORY = 1,   // Forest Stream Escape Victory (WIZ_SEG_YJK_VICTORY)
	WIZ_SC12_DEATH   = 2    // Graveyard Game Over Epitaph (WIZ_SEG_YJK_DEATH)
};

//=============================================================================
// PUBLIC VIDEO ENGINE FUNCTIONS
//=============================================================================

// Initialize V9958 SCREEN 12 (256x212, 19,268 YJK colors) with MSXgl Print fix
void Wiz_VideoInitScreen12(void);

// Draw a 4-pixel-aligned YJK color bar/fill in SCREEN 12
void Wiz_VideoDrawYJKBar(u8 x, u8 y, u16 width, u8 height, u8 luma, i8 j, i8 k);

// Render a full SCREEN 12 Event Screen (WIZ_SC12_SPLASH, WIZ_SC12_VICTORY, WIZ_SC12_DEATH)
// streaming the 160x100 YJK artwork from Mapper Segments 4..6 with themed YJK banners
void Wiz_VideoShowScreen12(u8 sc12Type, const c8* line1, const c8* line2, const c8* prompt);

// Initialize V9958 SCREEN 8 (256x212, 256 GRB colors), clear VRAM Page 0,
// upload 40 16x16 Map Tiles + 10 Treasure Icons into VRAM Page 1 (Y=256..319),
// and configure the 6x8 bitmap font.
void Wiz_VideoInitScreen8(void);

// Draw a double-beveled RPG window box in SCREEN 8 using hardware HMMV fills
void Wiz_VideoDrawBevelBox(u8 x1, u8 y1, u8 x2, u8 y2, u8 borderHi, u8 borderLo, u8 fillCol);

// Draw the complete 6-region SCREEN 8 Main Dungeon UI Window Skeleton
void Wiz_VideoDrawMainFrame(void);

// Run interactive Phase 3 Video Engine Showcase ([V] from Diagnostic Dashboard)
void Wiz_VideoRunShowcase(void);

