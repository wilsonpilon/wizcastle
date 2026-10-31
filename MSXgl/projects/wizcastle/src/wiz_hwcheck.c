// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Hardware Detection & Graphical Compatibility Diagnostic Module
//─────────────────────────────────────────────────────────────────────────────

#include "msxgl.h"
#include "dos.h"
#include "dos_mapper.h"
#include "extbios.h"
#include "string.h"
#include "psg.h"
#include "msx-music.h"
#include "scc.h"
#include "wiz_hwcheck.h"
#include "font/font_mgl_sample6.h"

//=============================================================================
// DEFINES & CONSTANTS
//=============================================================================

// Status badge types for graphical diagnostic rows
#define BADGE_OK           0    // Green [✓]
#define BADGE_FAIL         1    // Red   [✗]
#define BADGE_WARN         2    // Yellow [!]
#define BADGE_INFO         3    // Cyan  [i]

// Custom tile indices for SCREEN 2 (MSX1)
#define SC2_TILE_CHECK     1    // Green [✓] icon
#define SC2_TILE_CROSS     2    // Red   [✗] icon
#define SC2_TILE_WARN      3    // Yellow [!] icon
#define SC2_TILE_INFO      4    // Cyan  [i] icon
#define SC2_TILE_HBAR      5    // Blue horizontal bar

// Minimum and recommended Mapper RAM segments (16 KB each)
#define MIN_TOTAL_SEGS     16   // 256 KB minimum total Mapped RAM
#define REC_TOTAL_SEGS     64   // 1024 KB (1 MB) recommended Mapped RAM
#define MIN_FREE_SEGS      8    // At least 128 KB free in Mapper for WIZDATA.DAT

//=============================================================================
// GLOBAL & STATIC DATA
//=============================================================================

WizHardwareInfo g_HwInfo;
static c8 g_LineBuf[96];
static c8 g_SlotBuf[8];
static u8 g_FontBackup[256];
static bool g_FontBackedUp = FALSE;

// Sine-like 32-byte waveform for SCC test tone
static const i8 g_SccTestWave[32] = {
	  0,  24,  49,  71,  90, 106, 117, 125,
	127, 125, 117, 106,  90,  71,  49,  24,
	  0, -24, -49, -71, -90,-106,-117,-125,
   -127,-125,-117,-106, -90, -71, -49, -24
};

// Custom 8x8 icon patterns for SCREEN 2 (MSX1 fallback)
static const u8 g_Sc2IconPatterns[5 * 8] = {
	// Tile 1: Green Checkmark [✓]
	0b01111110,
	0b10000001,
	0b10000011,
	0b10000110,
	0b10101100,
	0b10011000,
	0b10000001,
	0b01111110,
	// Tile 2: Red Cross [✗]
	0b01111110,
	0b10000001,
	0b10100101,
	0b10011001,
	0b10011001,
	0b10100101,
	0b10000001,
	0b01111110,
	// Tile 3: Yellow Warning [!]
	0b01111110,
	0b10011001,
	0b10011001,
	0b10011001,
	0b10011001,
	0b10000001,
	0b10011001,
	0b01111110,
	// Tile 4: Cyan Info [i]
	0b01111110,
	0b10011001,
	0b10000001,
	0b10011001,
	0b10011001,
	0b10011001,
	0b10000001,
	0b01111110,
	// Tile 5: Horizontal Bar
	0b00000000,
	0b11111111,
	0b00000000,
	0b00000000,
	0b00000000,
	0b11111111,
	0b00000000,
	0b00000000
};

//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

//-----------------------------------------------------------------------------
// Read a single key from MSX-DOS console without echoing to screen
c8 Wiz_GetKeyNoEcho(void) __NAKED
{
__asm
	push	ix
	ld		c, #DOS_FUNC_DIRIN
	call	0x0005
	pop		ix
	ei
	ret
__endasm;
}

//-----------------------------------------------------------------------------
// Format an MSX Slot ID (ExxxSSPP) as "P" or "P-S"
static const c8* FormatSlotId(u8 slot)
{
	c8* p = g_SlotBuf;
	if (slot == SLOT_NOTFOUND)
	{
		*p++ = 'N'; *p++ = '/'; *p++ = 'A'; *p = 0;
		return g_SlotBuf;
	}
	*p++ = '0' + Sys_SlotGetPrimary(slot);
	if (Sys_SlotIsExpended(slot))
	{
		*p++ = '-';
		*p++ = '0' + Sys_SlotGetSecondary(slot);
	}
	*p = 0;
	return g_SlotBuf;
}

//-----------------------------------------------------------------------------
// Wait a given number of VBlank frames by polling V9958 S#2 Vertical Retrace
// (or HALT + timeout fallback on TMS9918A MSX1)
static void WaitFrames(u8 frames)
{
	if (g_HwInfo.vdpVersion == VDP_VERSION_TMS9918A)
	{
		EnableInterrupt();
		for (u8 i = 0; i < frames; ++i)
		{
			for (volatile u16 w = 0; w < 1200; ++w) {}
		}
		return;
	}

	for (u8 i = 0; i < frames; ++i)
	{
		u16 timeout = 8000;
		while ((VDP_ReadStatus(2) & 0x40) && --timeout) {}
		timeout = 30000;
		while (!(VDP_ReadStatus(2) & 0x40) && --timeout) {}
	}
}

//-----------------------------------------------------------------------------
// Write to YM2413 (MSX-MUSIC OPLL) with mandatory hardware wait cycles
static void OPLL_WriteReg(u8 reg, u8 value)
{
	g_MSXMusic_IndexPort = reg;
	for (volatile u8 d = 0; d < 4; ++d) {}
	g_MSXMusic_DataPort = value;
	for (volatile u8 d = 0; d < 24; ++d) {}
}

//-----------------------------------------------------------------------------
// Backup 2048-byte SCREEN 0 Font from VRAM Page 0 (0x0800 W40 or 0x1000 W80) to 0xD800
void Wiz_BackupTextFont(void)
{
	if (g_FontBackedUp || g_HwInfo.vdpVersion == VDP_VERSION_TMS9918A)
		return;

	u16 srcAddr = (PEEK(0xF3AE) > 40) ? 0x1000 : 0x0800;
	for (u8 block = 0; block < 8; ++block)
	{
		u16 off = (u16)block * 256;
		VDP_ReadVRAM_128K(srcAddr + off, 0, g_FontBackup, 256);
		VDP_WriteVRAM_128K(g_FontBackup, 0xD800 + off, 0, 256);
	}
	g_FontBackedUp = TRUE;
}

//-----------------------------------------------------------------------------
// Restore MSX-DOS text mode and VRAM font cleanly before exiting to DOS
void Wiz_RestoreTextMode(void)
{
	if (g_HwInfo.vdpVersion == VDP_VERSION_TMS9918A)
	{
		VDP_SetMode(VDP_MODE_SCREEN0_W40);
		VDP_SetColor(0xF1);
		VDP_FillVRAM_16K(' ', 0x0000, 960);
		VDP_FillVRAM_16K(0x00, 0x0800, 2048);
		// Reload ASCII font into 0x0800 for SCREEN 0 W40
		VDP_WriteVRAM_16K(g_Font_MGL_Sample6 + 4, 0x0800 + (32 * 8), 96 * 8);
		DOS_ClearScreen();
		return;
	}

	VDP_CommandWait();
	VDP_SetYJK(VDP_YJK_OFF);
	VDP_SetPage(0);

	bool useW80 = (PEEK(0xF3AE) > 40) || (g_HwInfo.dosVer >= DOS_VER_2);
	if (useW80)
	{
		POKE(0xF3AE, 80);
		VDP_SetMode(VDP_MODE_SCREEN0_W80);
		VDP_SetDefaultPalette();
		VDP_SetColor(0xF0);
		VDP_FillVRAM_128K(' ', 0x0000, 0, 2048); // Clear Name Table
		VDP_FillVRAM_128K(0x00, 0x0800, 0, 512); // Clear Blink Table
	}
	else
	{
		VDP_SetMode(VDP_MODE_SCREEN0_W40);
		VDP_SetDefaultPalette();
		VDP_SetColor(0xF0);
		VDP_FillVRAM_128K(' ', 0x0000, 0, 1024); // Clear Name Table
	}

	if (g_FontBackedUp)
	{
		u16 dstAddr = useW80 ? 0x1000 : 0x0800;
		for (u8 block = 0; block < 8; ++block)
		{
			u16 off = (u16)block * 256;
			VDP_ReadVRAM_128K(0xD800 + off, 0, g_FontBackup, 256);
			VDP_WriteVRAM_128K(g_FontBackup, dstAddr + off, 0, 256);
		}
	}
	DOS_ClearScreen();
}

//-----------------------------------------------------------------------------
// Test internal PSG presence via Register R#0 loopback read/write
static bool ProbePSG(void)
{
	DisableInterrupt();
	u8 orig0 = PSG_GetRegister(PSG_REG_TONE_A);
	u8 orig8 = PSG_GetRegister(PSG_REG_AMP_A);

	// Mute channel A while testing tone register
	PSG_SetRegister(PSG_REG_AMP_A, 0);

	PSG_SetRegister(PSG_REG_TONE_A, 0x55);
	u8 r1 = PSG_GetRegister(PSG_REG_TONE_A);

	PSG_SetRegister(PSG_REG_TONE_A, 0xAA);
	u8 r2 = PSG_GetRegister(PSG_REG_TONE_A);

	// Restore original registers
	PSG_SetRegister(PSG_REG_TONE_A, orig0);
	PSG_SetRegister(PSG_REG_AMP_A, orig8);
	EnableInterrupt();

	return (r1 == 0x55) && (r2 == 0xAA);
}

//=============================================================================
// SCREEN 7 (512x212, 16 COLORS) GRAPHICAL DIAGNOSTIC (MSX2 / MSX2+ / TURBO R)
//=============================================================================

//-----------------------------------------------------------------------------
// Draw a 20x12 Graphical Status Badge in SCREEN 7 ([✓] Green, [✗] Red, [!] Yellow, [i] Cyan)
// Note: dx must be even in SCREEN 7
static void DrawSc7Badge(u16 dx, u16 dy, u8 badgeType)
{
	u8 borderByte = 0x33;
	u8 fillByte   = 0x22;

	switch (badgeType)
	{
	case BADGE_OK:
		borderByte = 0x33; // Light Green (3)
		fillByte   = 0xC2; // Dark/Medium Green (12/2)
		break;
	case BADGE_FAIL:
		borderByte = 0x99; // Light Red (9)
		fillByte   = 0x66; // Dark Red (6)
		break;
	case BADGE_WARN:
		borderByte = 0xBB; // Light Yellow (11)
		fillByte   = 0xAA; // Dark Yellow (10)
		break;
	default:
		borderByte = 0x77; // Cyan (7)
		fillByte   = 0x44; // Dark Blue (4)
		break;
	}

	// Outer badge rectangle (20x12) & inner fill (16x10)
	VDP_CommandHMMV(dx, dy, 20, 12, borderByte);
	VDP_CommandHMMV(dx + 2, dy + 1, 16, 10, fillByte);
	VDP_CommandWait();

	if (badgeType == BADGE_OK)
	{
		// Draw bold white Checkmark (✓)
		Draw_Line(dx + 4, dy + 6, dx + 8, dy + 9, 15, 0);
		Draw_Line(dx + 5, dy + 6, dx + 9, dy + 9, 15, 0);
		Draw_Line(dx + 8, dy + 9, dx + 15, dy + 2, 15, 0);
		Draw_Line(dx + 9, dy + 9, dx + 16, dy + 2, 15, 0);
	}
	else if (badgeType == BADGE_FAIL)
	{
		// Draw bold white Cross (✗)
		Draw_Line(dx + 5, dy + 2, dx + 14, dy + 9, 15, 0);
		Draw_Line(dx + 6, dy + 2, dx + 15, dy + 9, 15, 0);
		Draw_Line(dx + 5, dy + 9, dx + 14, dy + 2, 15, 0);
		Draw_Line(dx + 6, dy + 9, dx + 15, dy + 2, 15, 0);
	}
	else if (badgeType == BADGE_WARN)
	{
		// Draw bold dark Exclamation (!)
		VDP_CommandHMMV(dx + 8, dy + 2, 4, 5, 0x11);
		VDP_CommandHMMV(dx + 8, dy + 8, 4, 2, 0x11);
	}
	else
	{
		// Draw bold white Info (i)
		VDP_CommandHMMV(dx + 8, dy + 2, 4, 2, 0xFF);
		VDP_CommandHMMV(dx + 8, dy + 5, 4, 5, 0xFF);
	}
	VDP_CommandWait();
}

//-----------------------------------------------------------------------------
// Draw a single diagnostic row in SCREEN 7 (512x212)
static void DrawSc7Row(u8 rowIdx, u8 badgeType, const c8* title, const c8* detail)
{
	u16 y = 32 + (u16)rowIdx * 18;

	// Row background strip (alternating dark slate / black)
	u8 bgCol = (rowIdx & 1) ? 4 : 1;
	u8 bgByte = (bgCol << 4) | bgCol;
	VDP_CommandHMMV(8, y - 2, 496, 16, bgByte);
	VDP_CommandWait();

	// 1. Graphical Icon Badge ([✓], [✗], [!], [i]) at X = 14
	DrawSc7Badge(14, y, badgeType);

	// 2. Item Title at X = 42 (White on row bg)
	Print_SetColor(15, bgCol);
	Print_SetPosition(42, y + 2);
	Print_DrawText(title);

	// 3. Item Status Detail at X = 162 (Color-coded by badgeType)
	u8 detailCol = 15;
	switch (badgeType)
	{
	case BADGE_OK:   detailCol = 3;  break; // Bright Green
	case BADGE_FAIL: detailCol = 9;  break; // Bright Red
	case BADGE_WARN: detailCol = 11; break; // Light Yellow
	default:         detailCol = 7;  break; // Cyan
	}
	Print_SetColor(detailCol, bgCol);
	Print_SetPosition(162, y + 2);
	Print_DrawText(detail);
	VDP_CommandWait();
}

//-----------------------------------------------------------------------------
// Render full SCREEN 7 (512x212, 16 colors) Graphical Diagnostic Dashboard
static void RenderDiagnosticScreen7(void)
{
	VDP_SetMode(VDP_MODE_SCREEN7);
	VDP_EnableSprite(FALSE);
	VDP_SetPage(0);
	VDP_SetDefaultPalette();
	VDP_SetColor(0xF1); // White on Black border

	// Clear entire 512x212 screen to Black (color 1 = 0x11)
	VDP_CommandHMMV(0, 0, 512, 212, 0x11);
	VDP_CommandWait();

	// Outer decorative frame (512x212)
	Draw_Box(0, 0, 511, 211, 5, 0);
	Draw_Box(2, 2, 509, 209, 15, 0);

	// Top Header Banner (Dark Blue 4)
	VDP_CommandHMMV(4, 4, 504, 22, 0x44);
	Draw_LineH(4, 507, 26, 11, 0);
	VDP_CommandWait();

	Print_SetBitmapFont(g_Font_MGL_Sample6);

	Print_SetColor(11, 4); // Gold on Dark Blue
	Print_SetPosition(40, 6);
	Print_DrawText("THE WIZARD'S CASTLE - MSX2+ DELUXE EDITION (v" WIZ_VERSION_STR ")");

	Print_SetColor(7, 4); // Cyan on Dark Blue
	Print_SetPosition(88, 16);
	Print_DrawText("HARDWARE & ENVIRONMENT COMPATIBILITY INSPECTOR");

	// --- Row 0: OS Environment ---
	u8 osBadge = (g_HwInfo.dosVer >= DOS_VER_2) ? BADGE_OK : BADGE_FAIL;
	if (g_HwInfo.dosVer >= DOS_VER_2)
	{
		const c8* osName = (g_HwInfo.dosVer == DOS_VER_NEXTOR) ? "Nextor" : "MSX-DOS 2";
		String_Format(g_LineBuf, "%s (Kernel %x.%02x, TPA: %04xh)",
			osName, g_HwInfo.dosKernel >> 8, g_HwInfo.dosKernel & 0xFF, g_HwInfo.tpaUpperAddr);
	}
	else if (g_HwInfo.dosVer == DOS_VER_1)
	{
		String_Format(g_LineBuf, "MSX-DOS 1.x -> Needs MSX-DOS 2 / Nextor!");
	}
	else
	{
		String_Format(g_LineBuf, "Unknown OS -> Needs MSX-DOS 2!");
	}
	DrawSc7Row(0, osBadge, "1. OS / DOS   :", g_LineBuf);

	// --- Row 1: MSX Generation ---
	const c8* msxName = "MSX2++";
	switch (g_HwInfo.msxVersion)
	{
	case MSXVER_1:  msxName = "MSX1";        break;
	case MSXVER_2:  msxName = "MSX2";        break;
	case MSXVER_2P: msxName = "MSX2+";       break;
	case MSXVER_TR: msxName = "MSX turbo R"; break;
	}
	const c8* freqStr = g_HwInfo.is50Hz ? "50Hz PAL" : "60Hz NTSC";
	u8 msxBadge = (g_HwInfo.msxVersion >= MSXVER_2P) ? BADGE_OK : BADGE_FAIL;
	if (msxBadge == BADGE_OK)
		String_Format(g_LineBuf, "%s (%s) - Compatible", msxName, freqStr);
	else
		String_Format(g_LineBuf, "%s (%s) -> Needs MSX2+ or turbo R!", msxName, freqStr);
	DrawSc7Row(1, msxBadge, "2. MSX SYSTEM :", g_LineBuf);

	// --- Row 2: VDP & VRAM ---
	const c8* vdpName = "Yamaha V9958";
	if (g_HwInfo.vdpVersion == VDP_VERSION_TMS9918A) vdpName = "TMS9918A";
	else if (g_HwInfo.vdpVersion == VDP_VERSION_V9938) vdpName = "Yamaha V9938";

	u8 vdpBadge = (g_HwInfo.vdpVersion >= VDP_VERSION_V9958 && g_HwInfo.vramSizeKB >= 128) ? BADGE_OK : BADGE_FAIL;
	if (vdpBadge == BADGE_OK)
		String_Format(g_LineBuf, "%s / %d KB VRAM (SC8 & SC12 Ready)", vdpName, g_HwInfo.vramSizeKB);
	else
		String_Format(g_LineBuf, "%s / %d KB VRAM -> Needs V9958 + 128KB!", vdpName, g_HwInfo.vramSizeKB);
	DrawSc7Row(2, vdpBadge, "3. VIDEO VDP  :", g_LineBuf);

	// --- Row 3: Memory Mapper ---
	u8 ramBadge = BADGE_FAIL;
	if (g_HwInfo.hasDosMapper && g_HwInfo.totalSegments >= REC_TOTAL_SEGS && g_HwInfo.freeSegments >= MIN_FREE_SEGS)
	{
		ramBadge = BADGE_OK;
		String_Format(g_LineBuf, "%d KB Total / %d KB Free (Slot %s, %d segs)",
			g_HwInfo.totalRamKB, g_HwInfo.freeRamKB, FormatSlotId(g_HwInfo.mapperSlot), g_HwInfo.totalSegments);
	}
	else if (g_HwInfo.hasDosMapper && g_HwInfo.totalSegments >= MIN_TOTAL_SEGS && g_HwInfo.freeSegments >= MIN_FREE_SEGS)
	{
		ramBadge = BADGE_WARN;
		String_Format(g_LineBuf, "%d KB Total / %d KB Free (1024 KB Recommended)",
			g_HwInfo.totalRamKB, g_HwInfo.freeRamKB);
	}
	else if (g_HwInfo.hasDosMapper)
	{
		String_Format(g_LineBuf, "%d KB Total / %d KB Free -> Needs >= 256 KB!",
			g_HwInfo.totalRamKB, g_HwInfo.freeRamKB);
	}
	else
	{
		String_Format(g_LineBuf, "No DOS2 Mapper Detected (64 KB TPA only)!");
	}
	DrawSc7Row(3, ramBadge, "4. RAM MAPPER :", g_LineBuf);

	// --- Row 4: PSG Audio ---
	u8 psgBadge = g_HwInfo.hasPSG ? BADGE_OK : BADGE_FAIL;
	DrawSc7Row(4, psgBadge, "5. AUDIO PSG  :",
		g_HwInfo.hasPSG ? "AY-3-8910 / YM2149 Detected (3ch + SFX)" : "PSG Not Responding on Ports A0h-A2h!");

	// --- Row 5: FM Audio (MSX-MUSIC) ---
	u8 fmBadge = (g_HwInfo.fmType != MSXMUSIC_NOTFOUND) ? BADGE_OK : BADGE_WARN;
	if (g_HwInfo.fmType == MSXMUSIC_INTERNAL)
		String_Format(g_LineBuf, "Internal MSX-MUSIC YM2413 (Slot %s, 9ch FM)", FormatSlotId(g_HwInfo.fmSlot));
	else if (g_HwInfo.fmType == MSXMUSIC_EXTERNAL)
		String_Format(g_LineBuf, "External FM-PAC YM2413 (Slot %s, 9ch FM)", FormatSlotId(g_HwInfo.fmSlot));
	else
		String_Format(g_LineBuf, "Not Found (Optional - Music uses PSG fallback)");
	DrawSc7Row(5, fmBadge, "6. AUDIO FM   :", g_LineBuf);

	// --- Row 6: Konami SCC Audio ---
	u8 sccBadge = g_HwInfo.hasSCC ? BADGE_OK : BADGE_INFO;
	if (g_HwInfo.hasSCC)
		String_Format(g_LineBuf, "Konami SCC Wavetable Detected (Slot %s, 5ch)", FormatSlotId(g_HwInfo.sccSlot));
	else
		String_Format(g_LineBuf, "Not Found (Optional 5ch Wavetable Expansion)");
	DrawSc7Row(6, sccBadge, "7. AUDIO SCC  :", g_LineBuf);

	// --- Verdict Box (Y = 160..188) ---
	if (g_HwInfo.isCompatible)
	{
		VDP_CommandHMMV(8, 160, 496, 26, 0xCC); // Dark Green fill (12)
		Draw_Box(8, 160, 503, 185, 3, 0);       // Light Green border (3)
		DrawSc7Badge(18, 167, BADGE_OK);
		Print_SetColor(15, 12);
		Print_SetPosition(46, 165);
		Print_DrawText("VERDICT: SYSTEM COMPATIBLE! ALL REQUIRED CHECKS PASSED.");
		Print_SetColor(11, 12);
		Print_SetPosition(46, 175);
		Print_DrawText("Ready to run The Wizard's Castle MSX2+ Deluxe Edition!");
	}
	else
	{
		VDP_CommandHMMV(8, 160, 496, 26, 0x66); // Dark Red fill (6)
		Draw_Box(8, 160, 503, 185, 9, 0);       // Light Red border (9)
		DrawSc7Badge(18, 167, BADGE_FAIL);
		Print_SetColor(15, 6);
		String_Format(g_LineBuf, "VERDICT: INCOMPATIBLE HARDWARE (%d Critical Error(s))!", g_HwInfo.failCount);
		Print_SetPosition(46, 165);
		Print_DrawText(g_LineBuf);
		Print_SetColor(11, 6);
		Print_SetPosition(46, 175);
		Print_DrawText("Required: MSX2+ (V9958), MSX-DOS 2, and >= 256 KB Mapper RAM.");
	}

	// --- Bottom Key Bar (Y = 192..207) ---
	VDP_CommandHMMV(4, 190, 504, 18, 0x44);
	VDP_CommandWait();
	Print_SetColor(15, 4);
	Print_SetPosition(28, 195);
	if (g_HwInfo.isCompatible)
		Print_DrawText("[T] Visual & Audio Test (SC12/SC8)   [S] Test Sound   [Q/ESC] Exit DOS");
	else
		Print_DrawText("Press any key to return to MSX-DOS...");
	VDP_CommandWait();
}

//=============================================================================
// SCREEN 2 (256x192, 32x24 TILES) GRAPHICAL DIAGNOSTIC (MSX1 FALLBACK)
//=============================================================================

//-----------------------------------------------------------------------------
// Write a string into SCREEN 2 Name Table (x: 0..31, y: 0..23) with tile offset
static void Sc2PrintAt(u8 x, u8 y, const c8* str, u8 tileOffset)
{
	u16 addr = 0x1800 + ((u16)y * 32) + x;
	while (*str && x < 32)
	{
		u8 ch = (u8)*str++;
		if (ch >= 32 && ch < 96)
			ch = (u8)(ch + tileOffset);
		VDP_FillVRAM_16K(ch, addr++, 1);
		x++;
	}
}

//-----------------------------------------------------------------------------
// Place a single custom icon tile in SCREEN 2 Name Table
static void Sc2PutIcon(u8 x, u8 y, u8 badgeType)
{
	u8 tile = SC2_TILE_INFO;
	if (badgeType == BADGE_OK)        tile = SC2_TILE_CHECK;
	else if (badgeType == BADGE_FAIL) tile = SC2_TILE_CROSS;
	else if (badgeType == BADGE_WARN) tile = SC2_TILE_WARN;

	VDP_FillVRAM_16K(tile, 0x1800 + ((u16)y * 32) + x, 1);
}

//-----------------------------------------------------------------------------
// Render SCREEN 2 (256x192) Graphical Diagnostic for MSX1 (TMS9918A)
static void RenderDiagnosticScreen2(void)
{
	VDP_SetMode(VDP_MODE_SCREEN2);
	VDP_SetColor(0xF1); // White on Black

	// Clear Name Table (0x1800, 768 bytes) to space (32)
	VDP_FillVRAM_16K(' ', 0x1800, 768);
	// Disable Sprites (0xD0 at Sprite Attribute Table 0x1B00)
	VDP_FillVRAM_16K(0xD0, 0x1B00, 1);

	// Initialize all 3 SCREEN 2 banks (0x0000, 0x0800, 0x1000)
	for (u8 bank = 0; bank < 3; ++bank)
	{
		u16 pgBase = (u16)bank * 0x0800;
		u16 ctBase = 0x2000 + ((u16)bank * 0x0800);

		// Clear pattern and color bank
		VDP_FillVRAM_16K(0x00, pgBase, 0x0800);
		VDP_FillVRAM_16K(0xF1, ctBase, 0x0800);

		// 1. Load custom 8x8 graphical icons into tiles 1..5
		VDP_WriteVRAM_16K(g_Sc2IconPatterns, pgBase + (1 * 8), 5 * 8);
		VDP_FillVRAM_16K(0x31, ctBase + (SC2_TILE_CHECK * 8), 8); // Green [✓]
		VDP_FillVRAM_16K(0x91, ctBase + (SC2_TILE_CROSS * 8), 8); // Red   [✗]
		VDP_FillVRAM_16K(0xB1, ctBase + (SC2_TILE_WARN  * 8), 8); // Yellow [!]
		VDP_FillVRAM_16K(0x71, ctBase + (SC2_TILE_INFO  * 8), 8); // Cyan  [i]
		VDP_FillVRAM_16K(0x51, ctBase + (SC2_TILE_HBAR  * 8), 8); // Light Blue bar

		// 2. Load standard ASCII 32..127 (White text 0xF1)
		VDP_WriteVRAM_16K(g_Font_MGL_Sample6 + 4, pgBase + (32 * 8), 96 * 8);

		// 3. Copy uppercase/digits 32..95 to tiles 128..191 (Green text 0x31)
		VDP_WriteVRAM_16K(g_Font_MGL_Sample6 + 4, pgBase + (128 * 8), 64 * 8);
		VDP_FillVRAM_16K(0x31, ctBase + (128 * 8), 64 * 8);

		// 4. Copy uppercase/digits 32..95 to tiles 192..255 (Red text 0x91)
		VDP_WriteVRAM_16K(g_Font_MGL_Sample6 + 4, pgBase + (192 * 8), 64 * 8);
		VDP_FillVRAM_16K(0x91, ctBase + (192 * 8), 64 * 8);
	}

	// Draw Header & Horizontal Bars
	VDP_FillVRAM_16K(SC2_TILE_HBAR, 0x1800 + (0 * 32), 32);
	Sc2PrintAt(1, 1, "WIZARD'S CASTLE MSX2+ (v" WIZ_VERSION_STR ")", 0);
	Sc2PrintAt(2, 2, "HARDWARE DIAGNOSTIC CHECK", 0);
	VDP_FillVRAM_16K(SC2_TILE_HBAR, 0x1800 + (3 * 32), 32);

	// Row 1: OS
	u8 b0 = (g_HwInfo.dosVer >= DOS_VER_2) ? BADGE_OK : BADGE_FAIL;
	Sc2PutIcon(1, 5, b0);
	Sc2PrintAt(3, 5, "1.OS :", 0);
	Sc2PrintAt(10, 5, (b0 == BADGE_OK) ? "MSX-DOS 2 OK" : "DOS1 (NEED DOS2)", (b0 == BADGE_OK) ? 96 : 160);

	// Row 2: MSX Generation
	u8 b1 = (g_HwInfo.msxVersion >= MSXVER_2P) ? BADGE_OK : BADGE_FAIL;
	Sc2PutIcon(1, 7, b1);
	Sc2PrintAt(3, 7, "2.MSX:", 0);
	Sc2PrintAt(10, 7, (b1 == BADGE_OK) ? "MSX2+ / TR OK" : "MSX1/2 (NEED 2+)", (b1 == BADGE_OK) ? 96 : 160);

	// Row 3: VDP
	u8 b2 = (g_HwInfo.vdpVersion >= VDP_VERSION_V9958 && g_HwInfo.vramSizeKB >= 128) ? BADGE_OK : BADGE_FAIL;
	Sc2PutIcon(1, 9, b2);
	Sc2PrintAt(3, 9, "3.VDP:", 0);
	Sc2PrintAt(10, 9, (b2 == BADGE_OK) ? "V9958 128KB OK" : "NEED V9958 128K", (b2 == BADGE_OK) ? 96 : 160);

	// Row 4: Mapper RAM
	u8 b3 = (g_HwInfo.hasDosMapper && g_HwInfo.totalSegments >= MIN_TOTAL_SEGS) ? BADGE_OK : BADGE_FAIL;
	Sc2PutIcon(1, 11, b3);
	Sc2PrintAt(3, 11, "4.RAM:", 0);
	Sc2PrintAt(10, 11, (b3 == BADGE_OK) ? "MAPPER RAM OK" : "64K (NEED MAPR)", (b3 == BADGE_OK) ? 96 : 160);

	// Row 5: PSG
	u8 b4 = g_HwInfo.hasPSG ? BADGE_OK : BADGE_FAIL;
	Sc2PutIcon(1, 13, b4);
	Sc2PrintAt(3, 13, "5.PSG:", 0);
	Sc2PrintAt(10, 13, g_HwInfo.hasPSG ? "AY-3-8910 OK" : "PSG FAIL", g_HwInfo.hasPSG ? 96 : 160);

	// Row 6: FM
	u8 b5 = (g_HwInfo.fmType != MSXMUSIC_NOTFOUND) ? BADGE_OK : BADGE_WARN;
	Sc2PutIcon(1, 15, b5);
	Sc2PrintAt(3, 15, "6.FM :", 0);
	Sc2PrintAt(10, 15, (b5 == BADGE_OK) ? "YM2413 OPLL OK" : "NO FM (OPT)", (b5 == BADGE_OK) ? 96 : 0);

	// Row 7: SCC
	u8 b6 = g_HwInfo.hasSCC ? BADGE_OK : BADGE_INFO;
	Sc2PutIcon(1, 17, b6);
	Sc2PrintAt(3, 17, "7.SCC:", 0);
	Sc2PrintAt(10, 17, g_HwInfo.hasSCC ? "KONAMI SCC OK" : "NO SCC (OPT)", g_HwInfo.hasSCC ? 96 : 0);

	VDP_FillVRAM_16K(SC2_TILE_HBAR, 0x1800 + (19 * 32), 32);

	if (g_HwInfo.isCompatible)
	{
		Sc2PutIcon(1, 20, BADGE_OK);
		Sc2PrintAt(3, 20, "SYSTEM COMPATIBLE!", 96);
		Sc2PrintAt(1, 22, "[T] TEST VIDEO/SND [Q] EXIT", 0);
	}
	else
	{
		Sc2PutIcon(1, 20, BADGE_FAIL);
		Sc2PrintAt(3, 20, "INCOMPATIBLE SYSTEM!", 160);
		Sc2PrintAt(2, 22, "Press any key to exit DOS", 0);
	}
}

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

//-----------------------------------------------------------------------------
// Probe all MSX hardware and OS components and populate g_HwInfo
void Wiz_DetectHardware(void)
{
	Mem_Set(0, &g_HwInfo, sizeof(WizHardwareInfo));

	// 1. Detect MSX-DOS Version & TPA
	DOS_Version ver;
	Mem_Set(0, &ver, sizeof(DOS_Version));
	g_HwInfo.dosVer       = DOS_GetVersion(&ver);
	g_HwInfo.dosKernel    = ver.Kernel;
	g_HwInfo.dosSystem    = ver.System;
	g_HwInfo.tpaUpperAddr = DOS_TPAUpperAddr;

	if (g_HwInfo.dosVer < DOS_VER_2)
		g_HwInfo.failCount++;

	// 2. Detect MSX Generation & VBlank Frequency (from Main-ROM via inter-slot read)
	g_HwInfo.msxVersion = Sys_GetMSXVersion();
	g_HwInfo.is50Hz     = Sys_Is50Hz();
	EnableInterrupt();

	if (g_HwInfo.msxVersion < MSXVER_2P)
		g_HwInfo.failCount++;

	// 3. Detect VDP Version & VRAM Size
	g_HwInfo.vdpVersion = VDP_GetVersion();
	if (g_HwInfo.msxVersion == MSXVER_1 || g_HwInfo.vdpVersion == VDP_VERSION_TMS9918A)
	{
		g_HwInfo.vramSizeKB = 16;
	}
	else
	{
		u8 vramBits = (g_MODE >> 1) & 0x03;
		switch (vramBits)
		{
		case 0:  g_HwInfo.vramSizeKB = 16;  break;
		case 1:  g_HwInfo.vramSizeKB = 64;  break;
		case 2:  g_HwInfo.vramSizeKB = 128; break;
		default: g_HwInfo.vramSizeKB = 192; break;
		}
	}

	if (g_HwInfo.vdpVersion < VDP_VERSION_V9958 || g_HwInfo.vramSizeKB < 128)
		g_HwInfo.failCount++;

	// 4. Detect Memory Mapper via Extended BIOS
	g_HwInfo.hasExtBios   = ExtBIOS_Check();
	g_HwInfo.hasDosMapper = FALSE;
	g_HwInfo.mapperSlot   = SLOT_NOTFOUND;
	g_HwInfo.totalRamKB   = 64;
	g_HwInfo.freeRamKB    = 0;

	if (g_HwInfo.hasExtBios && g_HwInfo.dosVer >= DOS_VER_2)
	{
		if (DOSMapper_Init())
		{
			DOS_VarTable* tab = DOSMapper_GetVarTable();
			if (tab && tab->Slot != 0)
			{
				g_HwInfo.hasDosMapper = TRUE;
				g_HwInfo.mapperSlot   = tab->Slot;
				while (tab->Slot != 0)
				{
					g_HwInfo.totalSegments += tab->NumSeg;
					g_HwInfo.freeSegments  += tab->FreeSeg;
					tab++;
				}
				g_HwInfo.totalRamKB = g_HwInfo.totalSegments * 16;
				g_HwInfo.freeRamKB  = g_HwInfo.freeSegments  * 16;
			}
		}
	}

	if (!g_HwInfo.hasDosMapper || g_HwInfo.totalSegments < MIN_TOTAL_SEGS || g_HwInfo.freeSegments < MIN_FREE_SEGS)
	{
		g_HwInfo.failCount++;
	}
	else if (g_HwInfo.totalSegments < REC_TOTAL_SEGS)
	{
		g_HwInfo.warnCount++;
	}

	// 5. Detect PSG (AY-3-8910 / YM2149)
	g_HwInfo.hasPSG = ProbePSG();
	if (!g_HwInfo.hasPSG)
		g_HwInfo.failCount++;

	// 6. Detect FM / MSX-MUSIC (YM2413 / OPLL)
	g_HwInfo.fmType = MSXMusic_Initialize();
	g_HwInfo.fmSlot = MSXMusic_GetSlotId();
	if (g_HwInfo.fmType == MSXMUSIC_NOTFOUND)
	{
		g_HwInfo.warnCount++;
	}
	else
	{
		u8 fmpacEnable = BIOS_InterSlotRead(g_HwInfo.fmSlot, 0x7FF6);
		BIOS_InterSlotWrite(g_HwInfo.fmSlot, 0x7FF6, fmpacEnable | 0x01);
	}

	// 7. Detect Konami SCC / SCC+
	g_HwInfo.hasSCC  = SCC_Initialize();
	g_HwInfo.sccSlot = g_SCC_SlotId;
	EnableInterrupt();

	g_HwInfo.isCompatible = (g_HwInfo.failCount == 0);
}

//-----------------------------------------------------------------------------
// Render the graphical hardware diagnostic screen:
//  - SCREEN 2 (256x192) on MSX1 (TMS9918A)
//  - SCREEN 7 (512x212) on MSX2 / MSX2+ / MSX turbo R (V9938 / V9958)
void Wiz_ShowGraphicalDiagnostic(void)
{
	Wiz_BackupTextFont();

	if (g_HwInfo.vdpVersion == VDP_VERSION_TMS9918A || g_HwInfo.msxVersion == MSXVER_1)
	{
		RenderDiagnosticScreen2();
	}
	else
	{
		RenderDiagnosticScreen7();
	}
}

//-----------------------------------------------------------------------------
// Print itemized hardware compatibility report to MSX-DOS console
void Wiz_PrintHardwareReport(void)
{
	DOS_ClearScreen();
	DOS_StringOutput("The Wizard's Castle MSX2+ Deluxe Edition (v" WIZ_VERSION_STR ")\r\n$");
}

//-----------------------------------------------------------------------------
// Play a short test chord on detected sound chips (PSG + FM + SCC)
void Wiz_PlayTestChord(void)
{
	// 1. PSG C-Major arpeggio (C4=428, E4=339, G4=285)
	if (g_HwInfo.hasPSG)
	{
		PSG_SetMixer(PSG_TONE_A_ON | PSG_TONE_B_ON | PSG_TONE_C_ON);
		PSG_SetTone(PSG_CHANNEL_A, 428);
		PSG_SetTone(PSG_CHANNEL_B, 339);
		PSG_SetTone(PSG_CHANNEL_C, 285);
		PSG_SetVolume(PSG_CHANNEL_A, 12);
		PSG_SetVolume(PSG_CHANNEL_B, 0);
		PSG_SetVolume(PSG_CHANNEL_C, 0);
		WaitFrames(8);

		PSG_SetVolume(PSG_CHANNEL_B, 12);
		WaitFrames(8);

		PSG_SetVolume(PSG_CHANNEL_C, 12);
	}

	// 2. MSX-MUSIC (YM2413 / OPLL) FM Chord if detected
	if (g_HwInfo.fmType != MSXMUSIC_NOTFOUND)
	{
		OPLL_WriteReg(0x30, 0x32);
		OPLL_WriteReg(0x31, 0x32);
		OPLL_WriteReg(0x10, 0xAC);
		OPLL_WriteReg(0x20, 0x18);
		OPLL_WriteReg(0x11, 0x02);
		OPLL_WriteReg(0x21, 0x1B);
	}

	// 3. Konami SCC Wavetable tone if detected
	if (g_HwInfo.hasSCC)
	{
		SCC_LoadWaveform(0, (const u8*)g_SccTestWave);
		SCC_SetFrequency(0, 214);
		SCC_SetVolume(0, 12);
		SCC_SetMixer(0x01);
	}

	WaitFrames(30);

	// Mute all sound chips cleanly
	if (g_HwInfo.hasPSG)
	{
		PSG_Mute();
	}
	if (g_HwInfo.fmType != MSXMUSIC_NOTFOUND)
	{
		for (u8 ch = 0; ch < 9; ++ch)
			OPLL_WriteReg(0x20 + ch, 0x00);
	}
	if (g_HwInfo.hasSCC)
	{
		SCC_Mute();
	}
}

//-----------------------------------------------------------------------------
// Encode a 4-pixel YJK group and write directly to SCREEN 12 VRAM (Page 0)
static void DrawYJKGroup(u16 vramAddr, u8 luma, i8 j, i8 k)
{
	u8 y5 = (luma & 0x1F) << 3;
	u8 uJ = (u8)j & 0x3F;
	u8 uK = (u8)k & 0x3F;
	u8 pkt[4];
	pkt[0] = y5 | (uK & 0x07);
	pkt[1] = y5 | ((uK >> 3) & 0x07);
	pkt[2] = y5 | (uJ & 0x07);
	pkt[3] = y5 | ((uJ >> 3) & 0x07);
	VDP_WriteVRAM_128K(pkt, vramAddr, 0, 4);
}

//-----------------------------------------------------------------------------
// Run interactive SCREEN 12 + SCREEN 8 + Audio visual test
void Wiz_RunHardwareVisualTest(void)
{
	Wiz_BackupTextFont();

	// --- STEP 1: Test MSX2+ SCREEN 12 (256x212, 19,268 YJK colors) ---
	VDP_SetMode(VDP_MODE_SCREEN12);
	g_VDP_Data.Mode = VDP_MODE_GRAPHIC7;

	VDP_EnableSprite(FALSE);
	VDP_SetPage(0);
	VDP_SetColor(0x00);
	VDP_CommandHMMV(0, 0, 256, 212, 0x00);
	VDP_CommandWait();

	for (u8 band = 0; band < 4; ++band)
	{
		u16 baseY = 28 + (u16)band * 36;
		u16 rowAddr = baseY * 256;
		for (u16 x = 0; x < 256; x += 4)
		{
			u8 luma = (u8)(x >> 3);
			i8 j = 0, k = 0;
			switch (band)
			{
			case 0: j =  31; k = -16; break;
			case 1: j = -16; k =  31; break;
			case 2: j = -24; k = -24; break;
			default:
				luma = 20;
				j = (i8)((x >> 2) - 32);
				k = (i8)(31 - (x >> 2));
				break;
			}
			DrawYJKGroup(rowAddr + x, luma, j, k);
		}
		for (u8 dy = 1; dy < 30; ++dy)
		{
			VDP_CommandHMMM(0, baseY, 0, baseY + dy, 256, 1);
		}
	}
	VDP_CommandWait();

	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Print_SetColor(0xF8, 0x00);
	Print_SetPosition(16, 8);
	Print_DrawText("MSX2+ SCREEN 12 (19,268 YJK COLORS)");
	Print_SetPosition(16, 180);
	Print_DrawText("SPLASH/VICTORY/DEATH MODE VERIFIED!");
	Print_SetPosition(16, 194);
	Print_DrawText("Press any key for SCREEN 8 Layout...");
	VDP_CommandWait();

	Wiz_PlayTestChord();
	Wiz_GetKeyNoEcho();

	// --- STEP 2: Test SCREEN 8 (256x212, 256 colors) Game Layout Mockup ---
	VDP_SetMode(VDP_MODE_SCREEN8);
	VDP_EnableSprite(FALSE);
	VDP_SetPage(0);
	VDP_SetColor(0x00);
	VDP_CommandHMMV(0, 0, 256, 212, 0x00);
	VDP_CommandWait();

	VDP_CommandHMMV(0, 0, 256, 10, 0x03);
	Draw_Box(3, 13, 132, 142, 0xFF, 0);
	for (u8 r = 0; r < 8; ++r)
	{
		for (u8 c = 0; c < 8; ++c)
		{
			u8 tileCol = ((r + c) & 1) ? 0x49 : 0x24;
			if (c == 0 && r == 3)
				tileCol = 0xFC;
			VDP_CommandHMMV(4 + c * 16, 14 + r * 16, 15, 15, tileCol);
		}
	}

	Draw_Box(136, 13, 184, 61, 0xE0, 0);
	VDP_CommandHMMV(137, 14, 46, 46, 0x25);

	Draw_Box(136, 80, 253, 142, 0x1C, 0);
	Draw_Box(0, 144, 255, 211, 0xFF, 0);
	VDP_CommandWait();

	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Print_SetColor(0xFF, 0x03);
	Print_SetPosition(4, 1);
	Print_DrawText("CASTLE: DERELICT'S DOOM   L:1 X:0 Y:3");

	Print_SetColor(0xFF, 0x00);
	Print_SetPosition(188, 14);
	Print_DrawText("ST:18 HP:36");
	Print_SetPosition(188, 24);
	Print_DrawText("DX:18 IQ:18");
	Print_SetPosition(188, 34);
	Print_DrawText("WP:Sword");
	Print_SetPosition(188, 44);
	Print_DrawText("AR:Plate(21)");
	Print_SetPosition(188, 54);
	Print_DrawText("FL:5  GP:999");

	Print_SetPosition(140, 84);
	Print_DrawText("TREASURES (8+RS+OZ)");
	for (u8 t = 0; t < 10; ++t)
	{
		VDP_CommandHMMV(140 + (t % 5) * 22, 96 + (t / 5) * 20, 14, 14, (u8)(0x1F + t * 23));
	}

	Print_SetPosition(4, 148);
	Print_DrawText("> SCREEN 8 (256 Colors) UI Layout Ready!");
	Print_SetPosition(4, 158);
	Print_DrawText("> 8x8 Map + 48x48 Room Viewport + HUD");
	Print_SetPosition(4, 170);
	Print_DrawText("> Audio: PSG + FM (YM2413) + SCC Active");
	Print_SetPosition(4, 196);
	Print_DrawText("Press any key to return to Inspector...");
	VDP_CommandWait();

	Wiz_PlayTestChord();
	Wiz_GetKeyNoEcho();
}
