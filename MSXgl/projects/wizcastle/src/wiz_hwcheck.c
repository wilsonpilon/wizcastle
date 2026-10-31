// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀▀▀        │   ▄▄▄                ▄▄
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
static u8 g_FontBackup[256]; // Chunk buffer for backing up/restoring SCREEN 0 font in VRAM Page 1

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
// (100% immune to Z80 DI/EI state and includes hardware safety timeout)
static void WaitFrames(u8 frames)
{
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
// Backup 2048-byte SCREEN 0 Font from VRAM Page 0 (0x1000) to Page 1 (0x8000)
static void BackupScreen0Font(void)
{
	for (u8 block = 0; block < 8; ++block)
	{
		u16 off = (u16)block * 256;
		VDP_ReadVRAM_128K(0x1000 + off, 0, g_FontBackup, 256);
		VDP_WriteVRAM_128K(g_FontBackup, 0x8000 + off, 1, 256);
	}
}

//-----------------------------------------------------------------------------
// Restore 2048-byte SCREEN 0 Font from VRAM Page 1 (0x8000) to Page 0 (0x1000)
static void RestoreScreen0Font(void)
{
	for (u8 block = 0; block < 8; ++block)
	{
		u16 off = (u16)block * 256;
		VDP_ReadVRAM_128K(0x8000 + off, 1, g_FontBackup, 256);
		VDP_WriteVRAM_128K(g_FontBackup, 0x1000 + off, 0, 256);
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
	EnableInterrupt(); // DOS_InterSlotRead disables interrupts via RDSLT; re-enable!

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
		// Ensure OPLL I/O ports 7Ch/7Dh are enabled in 7FF6h for both internal & external FM
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
			" [FAIL] 4. Memory Mapper  : No DOS2 Memory Mapper Detected (64 KB only)\r\n$");
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 5: PSG (AY-3-8910 / YM2149) ---
	if (g_HwInfo.hasPSG)
	{
		DOS_StringOutput(" [ OK ] 5. Audio PSG      : AY-3-8910 / YM2149 Detected (3ch Tone + SFX)\r\n$");
	}
	else
	{
		DOS_StringOutput(" [FAIL] 5. Audio PSG      : PSG Not Responding on Ports A0h-A2h\r\n$");
	}

	// --- Item 6: FM / MSX-MUSIC (YM2413 / OPLL) ---
	if (g_HwInfo.fmType == MSXMUSIC_INTERNAL)
	{
		String_Format(g_LineBuf,
			" [ OK ] 6. Audio FM       : Internal MSX-MUSIC YM2413 (Slot %s, 9ch FM)\r\n$",
			FormatSlotId(g_HwInfo.fmSlot));
	}
	else if (g_HwInfo.fmType == MSXMUSIC_EXTERNAL)
	{
		String_Format(g_LineBuf,
			" [ OK ] 6. Audio FM       : External FM-PAC YM2413 (Slot %s, 9ch FM)\r\n$",
			FormatSlotId(g_HwInfo.fmSlot));
	}
	else
	{
		String_Format(g_LineBuf,
			" [WARN] 6. Audio FM       : MSX-MUSIC Not Found (Music falls back to PSG)\r\n$");
	}
	DOS_StringOutput(g_LineBuf);

	// --- Item 7: Konami SCC / SCC+ ---
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
		// Instrument #3 (Piano) + Volume 2 on FM Channel 0 & 1
		OPLL_WriteReg(0x30, 0x32);
		OPLL_WriteReg(0x31, 0x32);
		// F-Number for C4 (0xAC, Block 4, Key-On = 0x10)
		OPLL_WriteReg(0x10, 0xAC);
		OPLL_WriteReg(0x20, 0x18);
		// F-Number for G4 (0x02, Block 5, Key-On = 0x10)
		OPLL_WriteReg(0x11, 0x02);
		OPLL_WriteReg(0x21, 0x1B);
	}

	// 3. Konami SCC Wavetable tone if detected
	if (g_HwInfo.hasSCC)
	{
		SCC_LoadWaveform(0, (const u8*)g_SccTestWave);
		SCC_SetFrequency(0, 214); // C5 octave
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
		OPLL_WriteReg(0x20, 0x00);
		OPLL_WriteReg(0x21, 0x00);
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
	// 0. Backup current SCREEN 0 (Width 80) Font from VRAM Page 0 (0x1000) to Page 1 (0x8000)
	BackupScreen0Font();

	// --- STEP 1: Test MSX2+ SCREEN 12 (256x212, 19,268 YJK colors) ---
	VDP_SetMode(VDP_MODE_SCREEN12);
	// Compatibility note: MSXgl Print module switches on VDP_MODE_GRAPHIC7 (8-bpp layout),
	// whereas SCREEN 12 is GRAPHIC7 with VDP R#25 YJK=1 enabled.
	g_VDP_Data.Mode = VDP_MODE_GRAPHIC7;

	VDP_EnableSprite(FALSE);
	VDP_SetPage(0);
	VDP_SetColor(0x00);
	VDP_CommandHMMV(0, 0, 256, 212, 0x00);
	VDP_CommandWait();

	// Draw 4 smooth YJK color gradient bands (Red, Green, Blue, Rainbow Chroma)
	// First draw the first scanline of each band, then use VDP YMMM to replicate vertically!
	for (u8 band = 0; band < 4; ++band)
	{
		u16 baseY = 28 + (u16)band * 36;
		u16 rowAddr = baseY * 256;
		for (u16 x = 0; x < 256; x += 4)
		{
			u8 luma = (u8)(x >> 3); // 0..31
			i8 j = 0, k = 0;
			switch (band)
			{
			case 0: j =  31; k = -16; break; // Warm Red / Gold gradient
			case 1: j = -16; k =  31; break; // Emerald Green gradient
			case 2: j = -24; k = -24; break; // Deep Sapphire Blue gradient
			default:
				luma = 20;
				j = (i8)((x >> 2) - 32);
				k = (i8)(31 - (x >> 2));
				break;
			}
			DrawYJKGroup(rowAddr + x, luma, j, k);
		}
		// Replicate the scanline 30 times downward using fast VDP HMMM
		for (u8 dy = 1; dy < 30; ++dy)
		{
			VDP_CommandHMMM(0, baseY, 0, baseY + dy, 256, 1);
		}
	}
	VDP_CommandWait();

	// In YJK mode, byte 0xF8 has Luma=31 and Chroma=0 (pure white); 0x00 is pure black
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
	Print_DrawText("Press any key to return to MSX-DOS...");
	VDP_CommandWait();

	Wiz_PlayTestChord();
	Wiz_GetKeyNoEcho();

	// --- STEP 3: Restore MSX2+ 80-column text mode & VRAM font cleanly ---
	VDP_CommandWait();
	VDP_SetYJK(VDP_YJK_OFF);
	VDP_SetPage(0);
	VDP_SetMode(VDP_MODE_SCREEN0_W80);
	VDP_SetDefaultPalette();
	VDP_SetColor(0xF0);
	VDP_FillVRAM_128K(' ', 0x0000, 0, 2048); // Clear Name Table
	VDP_FillVRAM_128K(0x00, 0x0800, 0, 512); // Clear Blink Table
	RestoreScreen0Font();                    // Restore 2048-byte Font to 0x1000
	DOS_ClearScreen();
}
