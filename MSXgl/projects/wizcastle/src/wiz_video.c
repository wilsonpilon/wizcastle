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
// PUBLIC FUNCTIONS — ITEM 2 (v3.2): SCREEN 12 (19,268 YJK COLORS) EVENT SCREENS
//=============================================================================

//-----------------------------------------------------------------------------
// Initialize V9958 SCREEN 12 (256x212, 19,268 YJK colors) with MSXgl Print fix
void Wiz_VideoInitScreen12(void)
{
	VDP_SetMode(VDP_MODE_SCREEN12);
	// Critical MSXgl fix: Print_SetMode only handles VDP_MODE_GRAPHIC7 (8)
	g_VDP_Data.Mode = VDP_MODE_GRAPHIC7;

	VDP_EnableSprite(FALSE);
	VDP_SetPage(0);
	VDP_SetColor(0x00);
	VDP_CommandHMMV(0, 0, 256, 212, 0x00);
	VDP_CommandWait();

	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Print_SetColor(0xF8, 0x00); // Pure YJK White (Luma 31, J=0, K=0) on Black
}

//-----------------------------------------------------------------------------
// Draw a 4-pixel-aligned YJK color bar/fill in SCREEN 12
void Wiz_VideoDrawYJKBar(u8 x, u8 y, u16 width, u8 height, u8 luma, i8 j, i8 k)
{
	u8 alignedX = x & 0xFC;
	u16 alignedW = (width + 3) & 0xFFFC;
	u8 y5 = (luma & 0x1F) << 3;
	u8 uJ = (u8)j & 0x3F;
	u8 uK = (u8)k & 0x3F;
	u8 pkt[4];
	pkt[0] = y5 | (uK & 0x07);
	pkt[1] = y5 | ((uK >> 3) & 0x07);
	pkt[2] = y5 | (uJ & 0x07);
	pkt[3] = y5 | ((uJ >> 3) & 0x07);

	VDP_CommandWait();
	u16 baseRowAddr = ((u16)y << 8) + alignedX;
	for (u16 dx = 0; dx < alignedW; dx += 4)
	{
		VDP_WriteVRAM_128K(pkt, baseRowAddr + dx, 0, 4);
	}
	for (u8 dy = 1; dy < height; ++dy)
	{
		VDP_CommandHMMM(alignedX, y, alignedX, y + dy, alignedW, 1);
	}
	VDP_CommandWait();
}

//-----------------------------------------------------------------------------
// Render a full SCREEN 12 Event Screen (WIZ_SC12_SPLASH, WIZ_SC12_VICTORY, WIZ_SC12_DEATH)
// streaming the 160x100 YJK artwork from Mapper Segments 4..6 with themed YJK banners
void Wiz_VideoShowScreen12(u8 sc12Type, const c8* line1, const c8* line2, const c8* prompt)
{
	Wiz_VideoInitScreen12();

	u8 yjkSeg = WIZ_SEG_YJK_COMBAT;
	const c8* headerTitle = "THE WIZARD'S CASTLE - MSX2+ DELUXE";
	const c8* bannerSub   = "THE LEGEND OF ZOT'S ORB OF POWER";
	u8 barLuma = 12;
	i8 barJ = -8, barK = -10; // Royal Blue YJK tint

	if (sc12Type == WIZ_SC12_VICTORY)
	{
		yjkSeg      = WIZ_SEG_YJK_VICTORY;
		headerTitle = "GLORIOUS VICTORY! YOU ESCAPED ZOT!";
		bannerSub   = "THE ORB OF ZOT SHINES IN THE SUNLIGHT!";
		barLuma = 14; barJ = -12; barK = 18; // Emerald Green YJK tint
	}
	else if (sc12Type == WIZ_SC12_DEATH)
	{
		yjkSeg      = WIZ_SEG_YJK_DEATH;
		headerTitle = "GAME OVER - ANOTHER HERO FALLS...";
		bannerSub   = "REST IN PEACE BENEATH ZOT'S DUNGEON";
		barLuma = 12; barJ = 24; barK = -8;  // Crimson Red YJK tint
	}

	// 1. Top YJK Header Banner (Y=0..12) + Gold Divider Line (Y=13)
	Wiz_VideoDrawYJKBar(0, 0, 256, 12, barLuma, barJ, barK);
	Wiz_VideoDrawYJKBar(0, 12, 256, 2, 26, 14, 10); // Bright Gold YJK bar

	Print_SetColor(0xF8, 0x00);
	Print_SetPosition(12, 2);
	Print_DrawText(headerTitle);

	// 2. Ornate Frame (X=44..211, Y=17..122) + 160x100 YJK Painting from Mapper
	Wiz_VideoDrawYJKBar(44, 17, 168, 106, 22, 12, 8); // Gold outer frame
	Draw_Box(46, 19, 209, 120, 0xF8, 0);
	VDP_CommandWait();

	if (Wiz_MapperIsReady())
	{
		Wiz_MapperDrawYJKPanel(yjkSeg, 48, 20);
	}

	// 3. Narrative Scroll Panel (Y=126..209)
	Wiz_VideoDrawYJKBar(0, 126, 256, 12, barLuma, barJ, barK);
	Draw_Box(0, 126, 255, 209, 0xF8, 0);
	VDP_CommandWait();

	Print_SetColor(0xF8, 0x00);
	Print_SetPosition(12, 128);
	Print_DrawText(bannerSub);

	if (line1)
	{
		Print_SetPosition(8, 144);
		Print_DrawText(line1);
	}
	if (line2)
	{
		Print_SetPosition(8, 156);
		Print_DrawText(line2);
	}

	// Interactive mode selector hint
	Print_SetPosition(8, 174);
	Print_DrawText("[1] Splash  [2] Victory  [3] Game Over");

	if (prompt)
	{
		Wiz_VideoDrawYJKBar(4, 192, 248, 14, 8, -6, -8);
		Print_SetPosition(8, 195);
		Print_DrawText(prompt);
	}
	VDP_CommandWait();
}

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

	// --- STEP A (v3.2): SCREEN 12 Event Screens Showcase ---
	Wiz_VideoShowScreen12(
		WIZ_SC12_VICTORY,
		"You emerge from the Castle carrying the",
		"priceless ORB OF ZOT and the RUNESTAFF!",
		"Any other key -> SCREEN 8 Dungeon UI"
	);

	while (1)
	{
		c8 k = Wiz_GetKeyNoEcho();
		if (k == '1')
		{
			Wiz_VideoShowScreen12(
				WIZ_SC12_SPLASH,
				"Many cycles ago in the Kingdom of N'dic,",
				"the gnomic wizard Zot forged his ORB...",
				"Any other key -> SCREEN 8 Dungeon UI"
			);
		}
		else if (k == '2')
		{
			Wiz_VideoShowScreen12(
				WIZ_SC12_VICTORY,
				"You emerge from the Castle carrying the",
				"priceless ORB OF ZOT and the RUNESTAFF!",
				"Any other key -> SCREEN 8 Dungeon UI"
			);
		}
		else if (k == '3')
		{
			Wiz_VideoShowScreen12(
				WIZ_SC12_DEATH,
				"At the end of your noble quest, the",
				"monsters of Zot's Castle claimed you!",
				"Any other key -> SCREEN 8 Dungeon UI"
			);
		}
		else
		{
			break;
		}
	}

	// --- STEP B (v3.1): Initialize SCREEN 8 + VRAM Page 1 Cache + 6-Region UI ---
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

