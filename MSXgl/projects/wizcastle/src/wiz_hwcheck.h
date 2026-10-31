// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Hardware Detection & Compatibility Diagnostic Module
//─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "msxgl.h"
#include "wiz_version.h"

//=============================================================================
// STRUCTURES
//=============================================================================

typedef struct WizHardwareInfo
{
	// 1. Operating System (MSX-DOS)
	u8   dosVer;          // DOS_VER_NONE(0), DOS_VER_1(1), DOS_VER_2(2), DOS_VER_NEXTOR(3)
	u16  dosKernel;       // Kernel BCD version (e.g., 0x0220 = 2.20)
	u16  dosSystem;       // System BCD version
	u16  tpaUpperAddr;    // Top of TPA address at 0x0006

	// 2. MSX Machine Generation
	u8   msxVersion;      // 0=MSX1, 1=MSX2, 2=MSX2+, 3=MSX turbo R
	bool is50Hz;          // TRUE=50Hz (PAL), FALSE=60Hz (NTSC)

	// 3. Video Display Processor (VDP) & VRAM
	u8   vdpVersion;      // 0=TMS9918A, 1=V9938, 2=V9958
	u16  vramSizeKB;      // 16, 64, 128, or 192 KB

	// 4. Memory Mapper (RAM)
	bool hasExtBios;      // Extended BIOS hook valid (HOKVLD bit 0)
	bool hasDosMapper;    // MSX-DOS 2 Mapper support routines initialized
	u8   mapperSlot;      // Primary Memory Mapper slot ID
	u16  totalSegments;   // Total 16 KB segments across mappers
	u16  freeSegments;    // Free 16 KB segments available for allocation
	u16  totalRamKB;      // Total RAM in KB (e.g., 64, 128, 256, 512, 1024)
	u16  freeRamKB;       // Free RAM in KB

	// 5. Audio Chips
	bool hasPSG;          // Internal AY-3-8910 / YM2149 detected
	u8   fmType;          // MSXMUSIC_NOTFOUND(0), INTERNAL(1), EXTERNAL(2)
	u8   fmSlot;          // Slot ID of MSX-MUSIC (YM2413 / OPLL)
	bool hasSCC;          // Konami SCC / SCC+ detected
	u8   sccSlot;         // Slot ID of Konami SCC cartridge

	// 6. Compatibility Verdict
	u8   failCount;       // Critical requirements missing (prevents running)
	u8   warnCount;       // Optional/recommended items missing
	bool isCompatible;    // TRUE if failCount == 0
} WizHardwareInfo;

extern WizHardwareInfo g_HwInfo;

//=============================================================================
// FUNCTIONS
//=============================================================================

// Probe all MSX hardware and OS components and populate g_HwInfo
void Wiz_DetectHardware(void);

// Print itemized hardware compatibility report to MSX-DOS console
void Wiz_PrintHardwareReport(void);

// Play a short test chord on detected sound chips (PSG + FM + SCC)
void Wiz_PlayTestChord(void);

// Run interactive SCREEN 12 + SCREEN 8 + Audio visual test
void Wiz_RunHardwareVisualTest(void);
