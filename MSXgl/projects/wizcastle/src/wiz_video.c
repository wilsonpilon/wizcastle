// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Video Engine Implementation (SCREEN 8 Main UI & SCREEN 12 Event Screens)
//─────────────────────────────────────────────────────────────────────────────

#include "msxgl.h"
#include "vdp.h"
#include "draw.h"
#include "print.h"
#include "memory.h"
#include "string.h"
#include "wiz_version.h"
#include "wiz_hwcheck.h"
#include "wiz_mapper.h"
#include "wiz_video.h"

// External 6x8 bitmap font defined in wiz_hwcheck.c (font/font_mgl_sample6.h)
extern const u8 g_Font_MGL_Sample6[];

//=============================================================================
// PUBLIC FUNCTIONS — ITEM 1 (v3.1): SCREEN 8 INIT, VRAM PAGE 1 CACHE & UI FRAMES
//=============================================================================

//-----------------------------------------------------------------------------
// Initialize V9958 SCREEN 8 (256x212, 256 GRB colors), clear VRAM Page 0,
// upload 40 16x16 Map Tiles + 10 Treasure Icons into VRAM Page 1 (Y=256..319),
// and configure the 6x8 bitmap font.
void Wiz_VideoInitScreen8(void)
{
	VDP_SetMode(VDP_MODE_SCREEN8);
	VDP_EnableSprite(FALSE);
	VDP_SetPage(0);
	VDP_SetColor(WIZ_COL_BLACK);

	// Clear visible Page 0 (Y = 0..211) and off-screen buffer (Y = 212..255)
	VDP_CommandHMMV(0, 0, 256, 256, WIZ_COL_BLACK);
	VDP_CommandWait();

	// Upload 40 16x16 Dungeon Map Tiles + 10 16x16 Treasure Icons into VRAM Page 1
	if (Wiz_MapperIsReady())
	{
		Wiz_MapperUploadTilesToVRAMPage1();
	}

	// Initialize 6x8 bitmap font for SCREEN 8 text rendering
	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Print_SetColor(WIZ_COL_WHITE, WIZ_COL_BLACK);
}

//-----------------------------------------------------------------------------
// Draw a double-beveled RPG window box in SCREEN 8 using hardware HMMV fills
void Wiz_VideoDrawBevelBox(u8 x1, u8 y1, u8 x2, u8 y2, u8 borderHi, u8 borderLo, u8 fillCol)
{
	u16 w = (u16)(x2 - x1 + 1);
	u16 h = (u16)(y2 - y1 + 1);

	// Outer shadow / dark bevel
	VDP_CommandHMMV(x1, y1, w, h, borderLo);
	VDP_CommandWait();

	// Inner bright highlight border
	Draw_Box(x1, y1, x2, y2, borderHi, 0);
	VDP_CommandWait();

	// Interior fill
	if (w > 4 && h > 4)
	{
		VDP_CommandHMMV(x1 + 2, y1 + 2, w - 4, h - 4, fillCol);
		VDP_CommandWait();
	}
}

//-----------------------------------------------------------------------------
// Draw the complete 6-region SCREEN 8 Main Dungeon UI Window Skeleton
void Wiz_VideoDrawMainFrame(void)
{
	// 1. Top Title & Location Banner (0, 0 .. 255, 10)
	VDP_CommandHMMV(0, 0, 256, 10, WIZ_COL_NAVY);
	VDP_CommandHMMV(0, 10, 256, 1, WIZ_COL_GOLD);
	VDP_CommandWait();

	Print_SetColor(WIZ_COL_GOLD, WIZ_COL_NAVY);
	Print_SetPosition(4, 1);
	Print_DrawText("THE WIZARD'S CASTLE");
	Print_SetColor(WIZ_COL_WHITE, WIZ_COL_NAVY);
	Print_SetPosition(130, 1);
	Print_DrawText("MSX2+ UI v" WIZ_VERSION_STR);

	// 2. 8x8 Dungeon Floor Map Frame (X=2..133, Y=12..143)
	Wiz_VideoDrawBevelBox(
		WIZ_UI_MAP_BOX_X1, WIZ_UI_MAP_BOX_Y1,
		WIZ_UI_MAP_BOX_X2, WIZ_UI_MAP_BOX_Y2,
		WIZ_COL_GOLD, WIZ_COL_GOLD_DARK, WIZ_COL_BLACK
	);

	// Draw subtle 8x8 tile grid guidelines inside the 128x128 Map Frame
	for (u8 r = 0; r < 8; ++r)
	{
		for (u8 c = 0; c < 8; ++c)
		{
			u8 tx = WIZ_UI_MAP_X + (c << 4);
			u8 ty = WIZ_UI_MAP_Y + (r << 4);
			u8 cellCol = ((r + c) & 1) ? 0x24 : 0x00;
			VDP_CommandHMMV(tx, ty, 16, 16, cellCol);
		}
	}
	VDP_CommandWait();

	// 3. 48x48 Room & Combat Viewport Frame (X=135..184, Y=12..62)
	Wiz_VideoDrawBevelBox(
		WIZ_UI_VIEW_BOX_X1, WIZ_UI_VIEW_BOX_Y1,
		WIZ_UI_VIEW_BOX_X2, WIZ_UI_VIEW_BOX_Y2,
		WIZ_COL_ORANGE, WIZ_COL_RED_DARK, WIZ_COL_GRAY_DARK
	);

	// 3b. Viewport Label / Monster HP Strip (X=135..184, Y=64..76)
	Wiz_VideoDrawBevelBox(
		WIZ_UI_VIEW_LBL_X1, WIZ_UI_VIEW_LBL_Y1,
		WIZ_UI_VIEW_LBL_X2, WIZ_UI_VIEW_LBL_Y2,
		WIZ_COL_GRAY_LIGHT, WIZ_COL_GRAY_DARK, WIZ_COL_BLACK
	);

	// 4. Character Status HUD Panel (X=186..255, Y=12..76)
	Wiz_VideoDrawBevelBox(
		WIZ_UI_HUD_BOX_X1, WIZ_UI_HUD_BOX_Y1,
		WIZ_UI_HUD_BOX_X2, WIZ_UI_HUD_BOX_Y2,
		WIZ_COL_CYAN, WIZ_COL_NAVY, WIZ_COL_BLACK
	);

	// 5. 10-Treasure Tray Panel (X=135..255, Y=78..143)
	Wiz_VideoDrawBevelBox(
		WIZ_UI_TREAS_BOX_X1, WIZ_UI_TREAS_BOX_Y1,
		WIZ_UI_TREAS_BOX_X2, WIZ_UI_TREAS_BOX_Y2,
		WIZ_COL_GOLD, WIZ_COL_GOLD_DARK, WIZ_COL_BLACK
	);
	VDP_CommandHMMV(WIZ_UI_TREAS_BOX_X1 + 2, WIZ_UI_TREAS_BOX_Y1 + 2, 117, 9, WIZ_COL_GOLD_DARK);
	VDP_CommandWait();
	Print_SetColor(WIZ_COL_WHITE, WIZ_COL_GOLD_DARK);
	Print_SetPosition(WIZ_UI_TREAS_BOX_X1 + 6, WIZ_UI_TREAS_BOX_Y1 + 2);
	Print_DrawText("TREASURES (8+RS+OZ)");

	// 6. 8-Line Color Message Terminal Frame (X=0..255, Y=145..211)
	Wiz_VideoDrawBevelBox(
		WIZ_UI_TERM_BOX_X1, WIZ_UI_TERM_BOX_Y1,
		WIZ_UI_TERM_BOX_X2, WIZ_UI_TERM_BOX_Y2,
		WIZ_COL_GRAY_LIGHT, WIZ_COL_GRAY_DARK, WIZ_COL_BLACK
	);
}

//-----------------------------------------------------------------------------
// Run interactive Phase 3 Video Engine Showcase ([V] from Diagnostic Dashboard)
void Wiz_VideoRunShowcase(void)
{
	if (!Wiz_MapperIsReady())
	{
		Wiz_MapperInitAndLoad();
	}

	// Initialize SCREEN 8 + VRAM Page 1 Tile/Icon Cache + 6x8 Font
	Wiz_VideoInitScreen8();

	// Render the 6-Region Main Dungeon UI Skeleton
	Wiz_VideoDrawMainFrame();

	// Annotate the 6 UI regions and demonstrate VRAM Page 1 HMMM cache blits
	Print_SetColor(WIZ_COL_GOLD, WIZ_COL_BLACK);
	Print_SetPosition(14, 24);
	Print_DrawText("REGION 2: 8x8 MAP");
	Print_SetColor(WIZ_COL_CYAN, WIZ_COL_BLACK);
	Print_SetPosition(14, 36);
	Print_DrawText("128x128 px (16x16)");
	Print_SetColor(WIZ_COL_WHITE, WIZ_COL_BLACK);
	Print_SetPosition(14, 52);
	Print_DrawText("VRAM Page 1 Cache:");

	// Blit 6 sample 16x16 tiles + 6 treasure icons from VRAM Page 1 (Y=256..319)
	if (Wiz_MapperIsReady())
	{
		for (u8 i = 0; i < 6; ++i)
		{
			Wiz_MapperBlitTile16(OBJ_CASTLE_ENTRANCE + i, 16 + i * 18, 66);
			Wiz_MapperBlitTile16(OBJ_KOBOLD + i, 16 + i * 18, 86);
			Wiz_MapperBlitTreasureIcon16(i, 16 + i * 18, 106);
		}
		VDP_CommandWait();
	}

	// Region 3: Viewport (48x48) annotation
	Print_SetColor(WIZ_COL_ORANGE, WIZ_COL_GRAY_DARK);
	Print_SetPosition(139, 26);
	Print_DrawText("R3:VIEW");
	Print_SetPosition(145, 38);
	Print_DrawText("48x48");

	// Region 3b: Viewport Label strip
	Print_SetColor(WIZ_COL_WHITE, WIZ_COL_BLACK);
	Print_SetPosition(139, 66);
	Print_DrawText("R3b:BAR");

	// Region 4: HUD Panel annotation
	Print_SetColor(WIZ_COL_CYAN, WIZ_COL_BLACK);
	Print_SetPosition(192, 18);
	Print_DrawText("R4: HUD");
	Print_SetColor(WIZ_COL_WHITE, WIZ_COL_BLACK);
	Print_SetPosition(192, 30);
	Print_DrawText("ST/DX/IQ");
	Print_SetPosition(192, 40);
	Print_DrawText("HP/WP/AR");
	Print_SetPosition(192, 50);
	Print_DrawText("FL/LM/GP");
	Print_SetPosition(192, 60);
	Print_DrawText("CURSES");

	// Region 5: Treasure Tray (10 slots outline preview)
	for (u8 t = 0; t < 10; ++t)
	{
		u8 tx = 140 + (t % 5) * 22;
		u8 ty = 92 + (t / 5) * 24;
		Draw_Box(tx, ty, tx + 17, ty + 17, WIZ_COL_GRAY_MID, 0);
	}
	VDP_CommandWait();

	// Region 6: 8-Line Message Terminal annotation (max 40 cols per line)
	Print_SetColor(WIZ_COL_GREEN, WIZ_COL_BLACK);
	Print_SetPosition(4, 150);
	Print_DrawText("> [v3.1] SCREEN 8 Video Engine Ready!");
	Print_SetColor(WIZ_COL_WHITE, WIZ_COL_BLACK);
	Print_SetPosition(4, 160);
	Print_DrawText("> VRAM P0 (00000h): 256x212 Active UI");
	Print_SetPosition(4, 170);
	Print_DrawText("> VRAM P1 (10000h): 40 Tiles + 20 Gems");
	Print_SetColor(WIZ_COL_CYAN, WIZ_COL_BLACK);
	Print_SetPosition(4, 180);
	Print_DrawText("> 6 Beveled RPG Window Regions Active");
	Print_SetColor(WIZ_COL_GOLD, WIZ_COL_BLACK);
	Print_SetPosition(4, 196);
	Print_DrawText("Press any key -> Hardware Inspector...");
	VDP_CommandWait();

	Wiz_GetKeyNoEcho();
}

