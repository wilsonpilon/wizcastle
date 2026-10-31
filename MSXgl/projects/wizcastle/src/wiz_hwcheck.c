// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Hardware Detection & Compatibility Diagnostic Module
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
// GLOBAL DATA
//=============================================================================

WizHardwareInfo g_HwInfo;
static c8 g_LineBuf[96];
static c8 g_SlotBuf[8];

// Minimum and recommended Mapper RAM segments (16 KB each)
#define MIN_TOTAL_SEGS     16   // 256 KB minimum total Mapped RAM
#define REC_TOTAL_SEGS     64   // 1024 KB (1 MB) recommended Mapped RAM
#define MIN_FREE_SEGS      8    // At least 128 KB free in Mapper for WIZDATA.DAT

// Sine-like 32-byte waveform for SCC test tone
static const i8 g_SccTestWave[32] = {
	  0,  24,  49,  71,  90, 106, 117, 125,
	127, 125, 117, 106,  90,  71,  49,  24,
	  0, -24, -49, -71, -90,-106,-117,-125,
   -127,-125,-117,-106, -90, -71, -49, -24
};

//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

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
// Busy-wait a given number of VBlank interrupts
static void WaitFrames(u8 frames)
{
	for (u8 i = 0; i < frames; ++i)
	{
		Halt();
	}
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
		g_HwInfo.warnCount++;

	// 7. Detect Konami SCC / SCC+
	g_HwInfo.hasSCC  = SCC_Initialize();
	g_HwInfo.sccSlot = g_SCC_SlotId;

	g_HwInfo.isCompatible = (g_HwInfo.failCount == 0);
}

//-----------------------------------------------------------------------------
// Print itemized hardware compatibility report to MSX-DOS console
void Wiz_PrintHardwareReport(void)
{
	DOS_ClearScreen();

	DOS_StringOutput("======================================================================\r\n$");
	DOS_StringOutput("  THE WIZARD'S CASTLE - MSX2+ DELUXE EDITION (v" WIZ_VERSION_STR " Hardware Stub)\r\n$");
	DOS_StringOutput("  Based on Joseph R. Power (1980) & Daniel D. Miller's WinWiz\r\n$");
	DOS_StringOutput("======================================================================\r\n\r\n$");

	DOS_StringOutput("Checking MSX Hardware & Environment Compatibility:\r\n\r\n$");

	// --- Item 1: MSX-DOS Version ---
	if (g_HwInfo.dosVer >= DOS_VER_2)
	{
		const c8* osName = (g_HwInfo.dosVer == DOS_VER_NEXTOR) ? "Nextor DOS2" : "MSX-DOS 2";
		String_Format(g_LineBuf,
			" [ OK ] 1. OS Environment : %s (Kernel %x.%02x, TPA Top: %04xh)\r\n$",
			osName,
			g_HwInfo.dosKernel >> 8, g_HwInfo.dosKernel & 0xFF,
			g_HwInfo.tpaUpperAddr);
	}
	else if (g_HwInfo.dosVer == DOS_VER_1)
	{
		String_Format(g_LineBuf,
			" [FAIL] 1. OS Environment : MSX-DOS 1.x (Requires MSX-DOS 2 / Nextor)\r\n$");
	}
	else
	{
		String_Format(g_LineBuf,
			" [FAIL] 1. OS Environment : Unknown DOS (Requires MSX-DOS 2)\r\n$");
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 2: MSX Machine Generation ---
	const c8* msxName = "Unknown";
	switch (g_HwInfo.msxVersion)
	{
	case MSXVER_1:  msxName = "MSX1";        break;
	case MSXVER_2:  msxName = "MSX2";        break;
	case MSXVER_2P: msxName = "MSX2+";       break;
	case MSXVER_TR: msxName = "MSX turbo R"; break;
	default:        msxName = "MSX2++";      break;
	}
	const c8* freqStr = g_HwInfo.is50Hz ? "50Hz PAL" : "60Hz NTSC";

	if (g_HwInfo.msxVersion >= MSXVER_2P)
	{
		String_Format(g_LineBuf,
			" [ OK ] 2. MSX Generation : %s (%s)\r\n$",
			msxName, freqStr);
	}
	else
	{
		String_Format(g_LineBuf,
			" [FAIL] 2. MSX Generation : %s (%s) -> Requires MSX2+ or turbo R\r\n$",
			msxName, freqStr);
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 3: VDP & VRAM ---
	const c8* vdpName = "Unknown VDP";
	switch (g_HwInfo.vdpVersion)
	{
	case VDP_VERSION_TMS9918A: vdpName = "TMS9918A"; break;
	case VDP_VERSION_V9938:    vdpName = "Yamaha V9938"; break;
	case VDP_VERSION_V9958:    vdpName = "Yamaha V9958"; break;
	}

	if (g_HwInfo.vdpVersion >= VDP_VERSION_V9958 && g_HwInfo.vramSizeKB >= 128)
	{
		String_Format(g_LineBuf,
			" [ OK ] 3. Video & VRAM   : %s with %d KB VRAM (SC8 & SC12 Ready)\r\n$",
			vdpName, g_HwInfo.vramSizeKB);
	}
	else
	{
		String_Format(g_LineBuf,
			" [FAIL] 3. Video & VRAM   : %s with %d KB VRAM (Needs V9958 + 128KB)\r\n$",
			vdpName, g_HwInfo.vramSizeKB);
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 4: Memory Mapper ---
	if (g_HwInfo.hasDosMapper && g_HwInfo.totalSegments >= REC_TOTAL_SEGS && g_HwInfo.freeSegments >= MIN_FREE_SEGS)
	{
		String_Format(g_LineBuf,
			" [ OK ] 4. Memory Mapper  : %d KB Total / %d KB Free (Slot %s, %d segs)\r\n$",
			g_HwInfo.totalRamKB, g_HwInfo.freeRamKB,
			FormatSlotId(g_HwInfo.mapperSlot), g_HwInfo.totalSegments);
	}
	else if (g_HwInfo.hasDosMapper && g_HwInfo.totalSegments >= MIN_TOTAL_SEGS && g_HwInfo.freeSegments >= MIN_FREE_SEGS)
	{
		String_Format(g_LineBuf,
			" [WARN] 4. Memory Mapper  : %d KB Total / %d KB Free (1024 KB Recommended)\r\n$",
			g_HwInfo.totalRamKB, g_HwInfo.freeRamKB);
	}
	else if (g_HwInfo.hasDosMapper)
	{
		String_Format(g_LineBuf,
			" [FAIL] 4. Memory Mapper  : %d KB Total / %d KB Free (Needs >= 256 KB RAM)\r\n$",
			g_HwInfo.totalRamKB, g_HwInfo.freeRamKB);
	}
	else
	{
		String_Format(g_LineBuf,
			" [FAIL] 4. Memory Mapper  : 64 KB Standard RAM / No DOS2 Mapper Detected\r\n$");
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 5: PSG ---
	if (g_HwInfo.hasPSG)
	{
		String_Format(g_LineBuf,
			" [ OK ] 5. Audio PSG      : AY-3-8910 / YM2149 Detected (3ch + ayFX SFX)\r\n$");
	}
	else
	{
		String_Format(g_LineBuf,
			" [FAIL] 5. Audio PSG      : PSG Not Responding on Ports A0h-A2h\r\n$");
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 6: FM / MSX-MUSIC (YM2413 / OPLL) ---
	if (g_HwInfo.fmType == MSXMUSIC_INTERNAL)
	{
		String_Format(g_LineBuf,
			" [ OK ] 6. Audio FM (OPLL): Internal MSX-MUSIC YM2413 (Slot %s, 9ch FM)\r\n$",
			FormatSlotId(g_HwInfo.fmSlot));
	}
	else if (g_HwInfo.fmType == MSXMUSIC_EXTERNAL)
	{
		String_Format(g_LineBuf,
			" [ OK ] 6. Audio FM (OPLL): External FM-PAC / YM2413 (Slot %s, 9ch FM)\r\n$",
			FormatSlotId(g_HwInfo.fmSlot));
	}
	else
	{
		String_Format(g_LineBuf,
			" [WARN] 6. Audio FM (OPLL): MSX-MUSIC Not Found (Will use PSG-only BGM)\r\n$");
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 7: Konami SCC ---
	if (g_HwInfo.hasSCC)
	{
		String_Format(g_LineBuf,
			" [ OK ] 7. Audio SCC      : Konami SCC Wavetable Detected (Slot %s, 5ch)\r\n$",
			FormatSlotId(g_HwInfo.sccSlot));
	}
	else
	{
		String_Format(g_LineBuf,
			" [INFO] 7. Audio SCC      : Konami SCC Not Found (Optional Wavetable)\r\n$");
	}
	DOS_StringOutput(g_LineBuf);

	// --- Summary Verdict ---
	DOS_StringOutput("\r\n----------------------------------------------------------------------\r\n$");
	if (g_HwInfo.isCompatible)
	{
		String_Format(g_LineBuf,
			" VERDICT: [ COMPATIBLE ] (%d Critical Errors, %d Warnings)\r\n$",
			g_HwInfo.failCount, g_HwInfo.warnCount);
		DOS_StringOutput(g_LineBuf);
		DOS_StringOutput(" -> This MSX can run The Wizard's Castle MSX2+ Deluxe Edition!\r\n$");
	}
	else
	{
		String_Format(g_LineBuf,
			" VERDICT: [ INCOMPATIBLE ] (%d Critical Error(s), %d Warning(s))\r\n$",
			g_HwInfo.failCount, g_HwInfo.warnCount);
		DOS_StringOutput(g_LineBuf);
		DOS_StringOutput(" -> Game cannot start. Required: MSX2+, MSX-DOS 2, 128KB VRAM, Mapper.\r\n$");
	}
	DOS_StringOutput("----------------------------------------------------------------------\r\n$");
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
		PSG_SetVolume(PSG_CHANNEL_A, 11);
		WaitFrames(6);
		PSG_SetVolume(PSG_CHANNEL_B, 11);
		WaitFrames(6);
		PSG_SetVolume(PSG_CHANNEL_C, 11);
	}

	// 2. MSX-MUSIC (YM2413 / OPLL) FM Chord if detected
	if (g_HwInfo.fmType != MSXMUSIC_NOTFOUND)
	{
		// Instrument #3 (Piano) + Volume 2 on FM Channel 0 & 1
		MSXMusic_SetRegister(0x30, 0x32);
		MSXMusic_SetRegister(0x31, 0x32);
		// F-Number for C4 (0xAC, Block 4, Key-On = 0x10)
		MSXMusic_SetRegister(0x10, 0xAC);
		MSXMusic_SetRegister(0x20, 0x18);
		// F-Number for G4 (0x02, Block 5, Key-On = 0x10)
		MSXMusic_SetRegister(0x11, 0x02);
		MSXMusic_SetRegister(0x21, 0x1B);
	}

	// 3. Konami SCC Wavetable tone if detected
	if (g_HwInfo.hasSCC)
	{
		SCC_LoadWaveform(0, (const u8*)g_SccTestWave);
		SCC_SetFrequency(0, 214); // C5 octave
		SCC_SetVolume(0, 12);
		SCC_SetMixer(0x01);
	}

	WaitFrames(25);

	// Mute all sound chips cleanly
	if (g_HwInfo.hasPSG)
	{
		PSG_Mute();
	}
	if (g_HwInfo.fmType != MSXMUSIC_NOTFOUND)
	{
		MSXMusic_SetRegister(0x20, 0x00);
		MSXMusic_SetRegister(0x21, 0x00);
		MSXMusic_Mute();
	}
	if (g_HwInfo.hasSCC)
	{
		SCC_Mute();
	}
}

//-----------------------------------------------------------------------------
// Run interactive SCREEN 12 + SCREEN 8 + Audio visual test
void Wiz_RunHardwareVisualTest(void)
{
	// --- STEP 1: Test MSX2+ SCREEN 12 (256x212, 19,268 YJK colors) ---
	VDP_SetMode(VDP_MODE_SCREEN12);
	VDP_EnableSprite(FALSE);
	VDP_SetColor(0x00);
	VDP_CommandHMMV(0, 0, 256, 212, 0x00);
	VDP_CommandWait();

	// Draw colorful YJK gradient bars across the screen
	for (u8 y = 24; y < 168; y += 4)
	{
		for (u16 x = 0; x < 256; x += 4)
		{
			u8 luma = (u8)((x >> 3) & 0x1F);
			u8 chroma = (u8)((y >> 3) & 0x07);
			u8 yjkByte = (luma << 3) | chroma;
			VDP_CommandHMMV(x, y, 4, 4, yjkByte);
		}
	}
	VDP_CommandWait();

	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Print_SetColor(0xFF, 0x00);
	Print_SetPosition(16, 6);
	Print_DrawText("MSX2+ SCREEN 12 (19,268 YJK COLORS)");
	Print_SetPosition(16, 184);
	Print_DrawText("SPLASH/VICTORY/DEATH MODE VERIFIED!");
	Print_SetPosition(16, 196);
	Print_DrawText("Press any key for SCREEN 8 Layout...");

	Wiz_PlayTestChord();
	DOS_CharInput();

	// --- STEP 2: Test SCREEN 8 (256x212, 256 colors) Game Layout Mockup ---
	VDP_SetMode(VDP_MODE_SCREEN8);
	VDP_EnableSprite(FALSE);
	VDP_SetColor(0x00);
	VDP_CommandHMMV(0, 0, 256, 212, 0x00);
	VDP_CommandWait();

	// Top title bar
	VDP_CommandHMMV(0, 0, 256, 10, 0x03); // Dark blue bar
	// Left 8x8 Map frame (128x128)
	Draw_Box(3, 13, 132, 142, 0xFF, 0);
	for (u8 r = 0; r < 8; ++r)
	{
		for (u8 c = 0; c < 8; ++c)
		{
			u8 tileCol = ((r + c) & 1) ? 0x49 : 0x24;
			if (c == 0 && r == 3)
				tileCol = 0xFC; // Player start (0,3) highlighted
			VDP_CommandHMMV(4 + c * 16, 14 + r * 16, 15, 15, tileCol);
		}
	}

	// Room Viewport frame (48x48)
	Draw_Box(136, 13, 184, 61, 0xE0, 0);
	VDP_CommandHMMV(137, 14, 46, 46, 0x25);

	// Treasures & HUD boxes
	Draw_Box(136, 80, 253, 142, 0x1C, 0);
	Draw_Box(0, 144, 255, 211, 0xFF, 0);

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
	Print_DrawText("Press any key to return to MSX-DOS...");

	Wiz_PlayTestChord();
	DOS_CharInput();

	// Restore MSX2+ 80-column text mode cleanly before returning to DOS
	VDP_SetMode(VDP_MODE_SCREEN0_W80);
	DOS_ClearScreen();
}
