// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  MSX-DOS 2 Memory Mapper & Asset Bank Manager (WIZDATA.DAT)
//─────────────────────────────────────────────────────────────────────────────

#include "msxgl.h"
#include "dos.h"
#include "dos_mapper.h"
#include "vdp.h"
#include "draw.h"
#include "print.h"
#include "memory.h"
#include "wiz_mapper.h"

//=============================================================================
// STATIC VARIABLES
//=============================================================================

// Allocated MSX-DOS 2 Memory Mapper segments (7 x 16 KB = 112 KB)
static DOS_Segment g_WizSegs[WIZ_NUM_MAPPER_SEGS];

// Number of segments currently allocated
static u8 g_NumAllocSegs = 0;

// Default MSX-DOS 2 TPA segment mapped at Z80 Page 2 (0x8000-0xBFFF)
static u8 g_OrigSegPage2 = 0;

// Flag indicating WIZDATA.DAT has been loaded and verified in Mapper RAM
static bool g_MapperReady = FALSE;

// Safe 2 KB disk transfer buffer in Page 1 TPA (.bss, below 0x8000)
// Ensures MSX-DOS 2 BDOS _READ never writes directly to a banked Page 2 window
static u8 g_DiskBuf[2048];

//=============================================================================
// SAFE MSX-DOS 2 FILE I/O HELPERS (PRESERVING IX / IY)
//=============================================================================

//-----------------------------------------------------------------------------
// Open a file read-only via MSX-DOS 2 BDOS 43h (_OPEN), preserving IX
// Returns file handle (0..63) on success, or 0xFF on error.
static u8 Wiz_DOS2_Open(const c8* filename) __NAKED
{
	filename; // HL
__asm
	push	ix
	ex		de, hl          // DE = pointer to ASCIIZ filename
	ld		a, #0x01        // Open mode: read-only (no write)
	ld		c, #0x43        // BDOS 43h (_OPEN)
	call	0x0005
	pop		ix
	ei
	or		a               // A == 0 on success, B = file handle
	jr		nz, wiz_open_err
	ld		a, b            // Return file handle in A
	ret
wiz_open_err:
	ld		a, #0xFF        // Return 0xFF on error
	ret
__endasm;
}

//-----------------------------------------------------------------------------
// Read 2,048 bytes from handle into g_DiskBuf via MSX-DOS 2 BDOS 48h (_READ)
// Returns TRUE (0xFF) if read succeeded, FALSE (0x00) on error.
static bool Wiz_DOS2_ReadChunk(u8 handle) __NAKED
{
	handle; // A
__asm
	push	ix
	ld		b, a            // B = file handle
	ld		de, #_g_DiskBuf // DE = 2048-byte buffer in TPA Page 1
	ld		hl, #2048       // HL = 2048 bytes
	ld		c, #0x48        // BDOS 48h (_READ)
	call	0x0005
	pop		ix
	ei
	or		a               // A == 0 on success
	jr		nz, wiz_read_err
	ld		a, #0xFF        // Return TRUE
	ret
wiz_read_err:
	xor		a               // Return FALSE
	ret
__endasm;
}

//-----------------------------------------------------------------------------
// Close a file handle via MSX-DOS 2 BDOS 45h (_CLOSE), preserving IX
static void Wiz_DOS2_Close(u8 handle) __NAKED
{
	handle; // A
__asm
	push	ix
	ld		b, a            // B = file handle
	ld		c, #0x45        // BDOS 45h (_CLOSE)
	call	0x0005
	pop		ix
	ei
	ret
__endasm;
}

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

//-----------------------------------------------------------------------------
// Returns TRUE if WIZDATA.DAT has been allocated and loaded into Mapper RAM
bool Wiz_MapperIsReady(void)
{
	return g_MapperReady;
}

//-----------------------------------------------------------------------------
// Returns the number of allocated 16 KB segments
u8 Wiz_MapperGetAllocatedCount(void)
{
	return g_NumAllocSegs;
}

//-----------------------------------------------------------------------------
// Map one of our 7 logical segments (WIZ_SEG_*) into Z80 Page 2 (0x8000-0xBFFF)
void Wiz_MapperMapSeg(u8 logicalSeg)
{
	if (logicalSeg < g_NumAllocSegs)
	{
		DOSMapper_SetPage2(g_WizSegs[logicalSeg].Number);
	}
}

//-----------------------------------------------------------------------------
// Restore the original MSX-DOS 2 TPA segment into Z80 Page 2 (0x8000-0xBFFF)
void Wiz_MapperRestorePage2(void)
{
	if (g_OrigSegPage2 != 0)
	{
		DOSMapper_SetPage2(g_OrigSegPage2);
	}
}

//-----------------------------------------------------------------------------
// Free all allocated Memory Mapper segments and restore default TPA Page 2
void Wiz_MapperCleanup(void)
{
	Wiz_MapperRestorePage2();

	for (u8 i = 0; i < g_NumAllocSegs; ++i)
	{
		DOSMapper_Free(g_WizSegs[i].Number, g_WizSegs[i].Slot);
	}
	g_NumAllocSegs = 0;
	g_MapperReady  = FALSE;
}

//-----------------------------------------------------------------------------
// Allocate 7 Memory Mapper segments (112 KB) and stream WIZDATA.DAT from disk
// while rendering a graphical progress bar in SCREEN 7.
bool Wiz_MapperInitAndLoad(void)
{
	if (g_MapperReady)
		return TRUE;

	// Save the original TPA Page 2 segment so we can restore it around BDOS calls
	g_OrigSegPage2 = DOSMapper_GetPage2();
	g_NumAllocSegs = 0;

	// 1. Allocate 7 User segments (112 KB) in Primary Memory Mapper
	for (u8 i = 0; i < WIZ_NUM_MAPPER_SEGS; ++i)
	{
		if (!DOSMapper_Alloc(DOS_ALLOC_USER, DOS_SEGSLOT_PRIM | DOS_SEGSLOT_THISFIRST, &g_WizSegs[i]))
		{
			Wiz_MapperCleanup();
			return FALSE;
		}
		g_NumAllocSegs++;
	}

	// 2. Open WIZDATA.DAT from the current MSX-DOS 2 drive/directory
	u8 fh = Wiz_DOS2_Open("WIZDATA.DAT");
	if (fh == 0xFF)
	{
		Wiz_MapperCleanup();
		return FALSE;
	}

	// 3. Draw graphical loading bar overlay on SCREEN 7 (512x212)
	VDP_CommandHMMV(80, 92, 352, 34, 0x00);
	VDP_CommandWait();
	Draw_Box(80, 92, 431, 125, 15, 0);
	Print_SetColor(15, 0);
	Print_SetPosition(96, 97);
	Print_DrawText("LOADING WIZDATA.DAT INTO MAPPER (112 KB)...");
	Draw_Box(96, 110, 415, 119, 7, 0);
	VDP_CommandWait();

	// 4. Read 7 segments x 8 chunks (2,048 bytes each) = 114,688 bytes
	for (u8 seg = 0; seg < WIZ_NUM_MAPPER_SEGS; ++seg)
	{
		for (u8 chunk = 0; chunk < 8; ++chunk)
		{
			// Always keep original TPA Page 2 mapped during MSX-DOS 2 BDOS disk reads
			DOSMapper_SetPage2(g_OrigSegPage2);

			if (!Wiz_DOS2_ReadChunk(fh))
			{
				Wiz_DOS2_Close(fh);
				Wiz_MapperCleanup();
				return FALSE;
			}

			// Map target segment into Page 2 (0x8000-0xBFFF) and copy 2 KB chunk
			DOSMapper_SetPage2(g_WizSegs[seg].Number);
			u16 destAddr = 0x8000 + ((u16)chunk * 2048);
			Mem_Copy(g_DiskBuf, (void*)destAddr, 2048);
		}

		// Restore TPA Page 2 and advance the graphical progress bar
		DOSMapper_SetPage2(g_OrigSegPage2);
		u16 barWidth = (u16)(seg + 1) * 45;
		VDP_CommandHMMV(98, 112, barWidth, 6, 0xAA);
		VDP_CommandWait();
	}

	// 5. Close WIZDATA.DAT
	DOSMapper_SetPage2(g_OrigSegPage2);
	Wiz_DOS2_Close(fh);

	// 6. Verify the "WIZDATA1" integrity signature in Segment 2 at offset 0x3F00
	DOSMapper_SetPage2(g_WizSegs[WIZ_SEG_SPR32_2].Number);
	const c8* sig = (const c8*)(0x8000 + 0x3F00);
	bool sigOk = (sig[0] == 'W' && sig[1] == 'I' && sig[2] == 'Z' && sig[3] == 'D' &&
	              sig[4] == 'A' && sig[5] == 'T' && sig[6] == 'A' && sig[7] == '1');
	DOSMapper_SetPage2(g_OrigSegPage2);

	if (!sigOk)
	{
		Wiz_MapperCleanup();
		return FALSE;
	}

	g_MapperReady = TRUE;
	return TRUE;
}

//-----------------------------------------------------------------------------
// Stream a 160x100 SCREEN 12 YJK panel (WIZ_SEG_YJK_*) from Mapper to VRAM Page 0
// Note: destX must be a multiple of 4 (YJK 4-pixel packet alignment)
void Wiz_MapperDrawYJKPanel(u8 yjkSeg, u8 destX, u8 destY)
{
	if (!g_MapperReady || yjkSeg < WIZ_SEG_YJK_COMBAT || yjkSeg > WIZ_SEG_YJK_DEATH)
		return;

	VDP_CommandWait();
	Wiz_MapperMapSeg(yjkSeg);

	const u8* srcPtr = (const u8*)0x8000;
	u8 alignedX = destX & 0xFC;

	for (u8 row = 0; row < 100; ++row)
	{
		u16 vramAddr = ((u16)(destY + row) << 8) + alignedX;
		VDP_WriteVRAM_128K(srcPtr, vramAddr, 0, 160);
		srcPtr += 160;
	}

	Wiz_MapperRestorePage2();
}

//-----------------------------------------------------------------------------
// Upload all 40 16x16 Map Tiles, 10 Owned + 10 Unowned Treasure Icons, and
// 2 48x48 Dungeon Viewport Backdrops into SCREEN 8 VRAM Page 1 (Y=256..383)
void Wiz_MapperUploadTilesToVRAMPage1(void)
{
	if (!g_MapperReady)
		return;

	VDP_CommandWait();
	Wiz_MapperMapSeg(WIZ_SEG_MAP_TILES);

	// 1. Upload 40 16x16 Map Tiles from Segment 3 (0x8000) into VRAM Page 1 (destHigh = 1):
	//    Tiles 0..15  -> Y = 256..271 (offsetY = 0..15),  X = (id & 15) * 16
	//    Tiles 16..31 -> Y = 272..287 (offsetY = 16..31), X = (id & 15) * 16
	//    Tiles 32..39 -> Y = 288..303 (offsetY = 32..47), X = (id & 15) * 16
	for (u8 id = 0; id < OBJ_COUNT; ++id)
	{
		const u8* tilePtr = (const u8*)(0x8000 + ((u16)id << 8));
		u8 baseX = (id & 0x0F) << 4;
		u8 baseY = (id >> 4) << 4; // 0, 16, or 32 within Page 1

		for (u8 r = 0; r < 16; ++r)
		{
			u16 vramLow = ((u16)(baseY + r) << 8) + baseX;
			VDP_WriteVRAM_128K(tilePtr + (r << 4), vramLow, 1, 16);
		}
	}

	// 2. Upload 2 48x48 Dungeon Viewport Backdrops from Segment 3 (0x8000 + 0x2800) into VRAM Page 1:
	//    Backdrop 0 (Explore Gold)   -> Y = 336..383 (offsetY = 80..127), X = 0..47
	//    Backdrop 1 (Combat Crimson) -> Y = 336..383 (offsetY = 80..127), X = 48..95
	for (u8 bg = 0; bg < 2; ++bg)
	{
		const u8* bgPtr = (const u8*)(0x8000 + 0x2800 + ((u16)bg * 2304));
		u8 baseX = bg * 48;
		u8 baseY = 80; // Y = 256 + 80 = 336

		for (u8 r = 0; r < 48; ++r)
		{
			u16 vramLow = ((u16)(baseY + r) << 8) + baseX;
			VDP_WriteVRAM_128K(bgPtr + ((u16)r * 48), vramLow, 1, 48);
		}
	}

	// 3. Switch to Segment 2 (WIZ_SEG_SPR32_2) and upload 10 Owned + 10 Unowned 16x16 Treasure Icons:
	//    Owned   (0x8000 + 0x2000) -> Y = 304..319 (offsetY = 48..63), X = idx * 16
	//    Unowned (0x8000 + 0x2A00) -> Y = 320..335 (offsetY = 64..79), X = idx * 16
	Wiz_MapperMapSeg(WIZ_SEG_SPR32_2);
	for (u8 t = 0; t < 10; ++t)
	{
		const u8* ownedPtr   = (const u8*)(0x8000 + 0x2000 + ((u16)t << 8));
		const u8* unownedPtr = (const u8*)(0x8000 + 0x2A00 + ((u16)t << 8));
		u8 baseX = t << 4;

		for (u8 r = 0; r < 16; ++r)
		{
			u16 vramOwned   = ((u16)(48 + r) << 8) + baseX;
			u16 vramUnowned = ((u16)(64 + r) << 8) + baseX;
			VDP_WriteVRAM_128K(ownedPtr   + (r << 4), vramOwned,   1, 16);
			VDP_WriteVRAM_128K(unownedPtr + (r << 4), vramUnowned, 1, 16);
		}
	}

	Wiz_MapperRestorePage2();
}

//-----------------------------------------------------------------------------
// Hardware-blit a 16x16 Map Tile (objId 0..39) from VRAM Page 1 cache to Page 0 (destX, destY)
void Wiz_MapperBlitTile16(u8 objId, u8 destX, u8 destY)
{
	if (objId >= OBJ_COUNT)
		return;

	u16 srcX = (u16)(objId & 0x0F) << 4;
	u16 srcY = 256 + ((u16)(objId >> 4) << 4);
	VDP_CommandHMMM(srcX, srcY, destX, destY, 16, 16);
}

//-----------------------------------------------------------------------------
// Hardware-blit a 16x16 Owned Treasure Icon (treasIdx 0..9) from VRAM Page 1 cache to Page 0 (destX, destY)
void Wiz_MapperBlitTreasureIcon16(u8 treasIdx, u8 destX, u8 destY)
{
	if (treasIdx >= 10)
		return;

	u16 srcX = (u16)treasIdx << 4;
	u16 srcY = 256 + 48; // Y = 304 in VRAM Page 1
	VDP_CommandHMMM(srcX, srcY, destX, destY, 16, 16);
}

//-----------------------------------------------------------------------------
// Hardware-blit a 16x16 Unowned Treasure Silhouette (treasIdx 0..9) from VRAM Page 1 cache
void Wiz_MapperBlitUnownedTreasureIcon16(u8 treasIdx, u8 destX, u8 destY)
{
	if (treasIdx >= 10)
		return;

	u16 srcX = (u16)treasIdx << 4;
	u16 srcY = 256 + 64; // Y = 320 in VRAM Page 1
	VDP_CommandHMMM(srcX, srcY, destX, destY, 16, 16);
}

//-----------------------------------------------------------------------------
// Hardware-blit a 48x48 Dungeon Viewport Brick Background (0=Explore, 1=Combat) from VRAM Page 1 cache
void Wiz_MapperBlitViewportBg48(bool isCombat, u8 destX, u8 destY)
{
	u16 srcX = isCombat ? 48 : 0;
	u16 srcY = 256 + 80; // Y = 336 in VRAM Page 1
	VDP_CommandHMMM(srcX, srcY, destX, destY, 48, 48);
}

//-----------------------------------------------------------------------------
// Draw a 32x32 Sprite (objId 0..39) from Mapper Segments 0..2 into SCREEN 8 Page 0 (destX, destY)
void Wiz_MapperDrawSprite32(u8 objId, u8 destX, u8 destY)
{
	if (!g_MapperReady || objId >= OBJ_COUNT)
		return;

	VDP_CommandWait();

	u8 segIdx = WIZ_SEG_SPR32_0 + (objId >> 4);
	u16 offset = (u16)(objId & 0x0F) << 10; // 1,024 bytes per 32x32 sprite

	Wiz_MapperMapSeg(segIdx);
	const u8* sprPtr = (const u8*)(0x8000 + offset);

	for (u8 r = 0; r < 32; ++r)
	{
		u16 vramLow = ((u16)(destY + r) << 8) + destX;
		VDP_WriteVRAM_128K(sprPtr + ((u16)r << 5), vramLow, 0, 32);
	}

	Wiz_MapperRestorePage2();
}
