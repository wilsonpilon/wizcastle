// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  MSXgl Library Configuration
//─────────────────────────────────────────────────────────────────────────────
#pragma once

//-----------------------------------------------------------------------------
// BIOS MODULE
//-----------------------------------------------------------------------------

// Under MSX-DOS, Page 0 is RAM, so Main-ROM calls must use inter-slot calls
#define BIOS_CALL_MAINROM			BIOS_CALL_INTERSLOT
#define BIOS_CALL_SUBROM			BIOS_CALL_INTERSLOT
#define BIOS_CALL_DISKROM			BIOS_CALL_INTERSLOT

#define BIOS_USE_MAINROM			TRUE
#define BIOS_USE_VDP				TRUE
#define BIOS_USE_PSG				TRUE
#define BIOS_USE_SUBROM				TRUE
#define BIOS_USE_DISKROM			TRUE

//-----------------------------------------------------------------------------
// VDP MODULE
//-----------------------------------------------------------------------------

#define VDP_VRAM_ADDR				VDP_VRAM_ADDR_17
#define VDP_UNIT					VDP_UNIT_U16

#define VDP_USE_MODE_T1				TRUE	// Screen 0 Width 40
#define VDP_USE_MODE_G1				FALSE
#define VDP_USE_MODE_G2				TRUE	// Screen 2 (MSX1 256x192 Graphic 2)
#define VDP_USE_MODE_MC				FALSE
#define VDP_USE_MODE_T2				TRUE	// Screen 0 Width 80
#define VDP_USE_MODE_G3				FALSE
#define VDP_USE_MODE_G4				TRUE	// Screen 5
#define VDP_USE_MODE_G5				FALSE
#define VDP_USE_MODE_G6				TRUE	// Screen 7 (MSX2/2+ 512x212 Graphic 6)
#define VDP_USE_MODE_G7				TRUE	// Screen 8, 10, 11 & 12 (MSX2+)

#define VDP_USE_VRAM16K				TRUE
#define VDP_USE_SPRITE				TRUE
#define VDP_USE_COMMAND				TRUE
#define VDP_USE_CUSTOM_CMD			FALSE
#define VDP_AUTO_INIT				TRUE
#define VDP_USE_UNDOCUMENTED		FALSE
#define VDP_USE_VALIDATOR			TRUE
#define VDP_USE_DEFAULT_PALETTE		TRUE
#define VDP_USE_MSX1_PALETTE		FALSE
#define VDP_USE_DEFAULT_SETTINGS	TRUE
#define VDP_USE_16X16_SPRITE		TRUE
#define VDP_USE_RESTORE_S0			TRUE
#define VDP_USE_PALETTE16			TRUE

#define VDP_ISR_SAFE_MODE			VDP_ISR_SAFE_DEFAULT
#define VDP_INIT_50HZ				VDP_INIT_DEFAULT

//-----------------------------------------------------------------------------
// INPUT MODULE
//-----------------------------------------------------------------------------

#define INPUT_USE_JOYSTICK			TRUE
#define INPUT_USE_KEYBOARD			TRUE
#define INPUT_USE_MOUSE				FALSE
#define INPUT_USE_DETECT			FALSE
#define INPUT_USE_ISR_PROTECTION	TRUE
#define INPUT_JOY_UPDATE			FALSE
#define INPUT_HOLD_SIGNAL			FALSE
#define INPUT_KB_AS_JOYSTICK		FALSE
#define INPUT_KB_UPDATE				FALSE
#define INPUT_KB_UPDATE_MIN			0
#define INPUT_KB_UPDATE_MAX			8

//-----------------------------------------------------------------------------
// MEMORY MODULE
//-----------------------------------------------------------------------------

#define MEM_USE_VALIDATOR			FALSE
#define MEM_USE_FASTCOPY			FALSE
#define MEM_USE_FASTSET				FALSE
#define MEM_USE_DYNAMIC				FALSE
#define MEM_USE_BUILTIN				TRUE

//-----------------------------------------------------------------------------
// MSX-DOS MODULE
//-----------------------------------------------------------------------------

#define DOS_USE_FCB					TRUE
#define DOS_USE_HANDLE				TRUE
#define DOS_USE_UTILITIES			TRUE
#define DOS_USE_VALIDATOR			TRUE
#define DOS_USE_ERROR_HANDLER		TRUE
#define DOS_USE_BIOSCALL			TRUE

//-----------------------------------------------------------------------------
// PRINT MODULE
//-----------------------------------------------------------------------------

#define PRINT_USE_TEXT				TRUE
#define PRINT_USE_BITMAP			TRUE
#define PRINT_USE_VRAM				TRUE
#define PRINT_USE_SPRITE			FALSE
#define PRINT_USE_FX_SHADOW			FALSE
#define PRINT_USE_FX_OUTLINE		FALSE
#define PRINT_USE_2_PASS_FX			FALSE
#define PRINT_USE_GRAPH				TRUE
#define PRINT_USE_VALIDATOR			TRUE
#define PRINT_USE_UNIT				FALSE
#define PRINT_USE_FORMAT			TRUE
#define PRINT_USE_32B				FALSE
#define PRINT_USE_MULTIFONT			FALSE
#define PRINT_SKIP_SPACE			FALSE
#define PRINT_COLOR_NUM				8
#define PRINT_WIDTH					PRINT_WIDTH_6
#define PRINT_HEIGHT				PRINT_HEIGHT_8

//-----------------------------------------------------------------------------
// STRING MODULE
//-----------------------------------------------------------------------------

#define STRING_USE_FROM_INT8		TRUE
#define STRING_USE_FROM_UINT8		TRUE
#define STRING_USE_FROM_INT16		TRUE
#define STRING_USE_FROM_UINT16		TRUE
#define STRING_USE_FORMAT			TRUE
#define STRING_USE_INT32			FALSE

//-----------------------------------------------------------------------------
// AUDIO MODULE
//-----------------------------------------------------------------------------

#define PSG_CHIP					PSG_INTERNAL
#define PSG_ACCESS					PSG_DIRECT
#define PSG_USE_NOTES				FALSE
#define PSG_USE_EXTRA				TRUE
#define PSG_USE_RESUME				TRUE

#define MSXMUSIC_USE_RESUME			TRUE

#define SCC_USE_EXTA				TRUE
#define SCC_USE_RESUME				TRUE
#define SCC_SLOT_MODE				SCC_SLOT_AUTO

#define AYFX_BUFFER					AYFX_BUFFER_DEFAULT

#define LVGM_USE_PSG				TRUE
#define LVGM_USE_MSXMUSIC			TRUE
#define LVGM_USE_MSXAUDIO			FALSE
#define LVGM_USE_SCC				TRUE
#define LVGM_USE_SCCI				FALSE
#define LVGM_USE_PSG2				FALSE
#define LVGM_USE_OPL4				FALSE
#define LVGM_USE_NOTIFY				TRUE

//-----------------------------------------------------------------------------
// MATH MODULE
//-----------------------------------------------------------------------------

#define RANDOM_8_METHOD				RANDOM_8_ION
#define RANDOM_16_METHOD			RANDOM_16_XORSHIFT

//-----------------------------------------------------------------------------
// COMPRESS MODULE
//-----------------------------------------------------------------------------

#define ZX0_MODE					ZX0_MODE_STANDARD

//-----------------------------------------------------------------------------
// DEBUG & PROFILE
//-----------------------------------------------------------------------------

#define DEBUG_TOOL					DEBUG_DISABLE
#define PROFILE_TOOL				PROFILE_DISABLE
#define PROFILE_LEVEL				10

//-----------------------------------------------------------------------------
// OTHER MODULES (DEFAULTS TO SILENCE WARNINGS)
//-----------------------------------------------------------------------------

#define V9_USE_MODE_P1				FALSE
#define V9_USE_MODE_P2				FALSE
#define V9_USE_MODE_B0				FALSE
#define V9_USE_MODE_B1				FALSE
#define V9_USE_MODE_B2				FALSE
#define V9_USE_MODE_B3				FALSE
#define V9_USE_MODE_B4				FALSE
#define V9_USE_MODE_B5				FALSE
#define V9_USE_MODE_B6				FALSE
#define V9_USE_MODE_B7				FALSE
#define V9_INT_PROTECT				FALSE
#define V9_PALETTE_MODE				V9_PALETTE_RGB_24

#define PADDLE_USE_CALIB			FALSE

#define RTC_USE_CLOCK				TRUE
#define RTC_USE_CLOCK_EXTRA			FALSE
#define RTC_USE_SAVEDATA			FALSE
#define RTC_USE_SAVESIGNED			FALSE

#define SPRITEFX_USE_8x8			FALSE
#define SPRITEFX_USE_16x16			TRUE
#define SPRITEFX_USE_CROP			FALSE
#define SPRITEFX_USE_FLIP			FALSE
#define SPRITEFX_USE_MASK			FALSE
#define SPRITEFX_USE_ROTATE			FALSE

#define GAME_USE_STATE				TRUE
#define GAME_USE_VSYNC				TRUE
#define GAME_USE_LOOP				TRUE
#define GAME_USE_SYNC_50HZ			FALSE

#define PAWN_ID_PER_LAYER			FALSE
#define PAWN_USE_RT_LOAD			FALSE
#define PAWN_USE_SPRT_FX			FALSE
#define PAWN_SPRITE_SIZE			16
#define PAWN_BLEND_OFFSET			12
#define PAWN_USE_PHYSICS			FALSE
#define PAWN_UNIT					PAWN_UNIT_SCREEN
#define PAWN_BOUND_X				PAWN_BOUND_CUSTOM
#define PAWN_BOUND_Y				PAWN_BOUND_CUSTOM
#define PAWN_COL_DOWN				PAWN_COL_50
#define PAWN_COL_UP					PAWN_COL_50
#define PAWN_COL_RIGHT				PAWN_COL_50
#define PAWN_COL_LEFT				PAWN_COL_50
#define PAWN_BORDER_EVENT			PAWN_BORDER_NONE
#define PAWN_BORDER_BLOCK			PAWN_BORDER_NONE
#define PAWN_BORDER_MIN_Y			0
#define PAWN_BORDER_MAX_Y			211
#define PAWN_TILEMAP_WIDTH			32
#define PAWN_TILEMAP_HEIGHT			27
#define PAWN_TILEMAP_SRC			PAWN_TILEMAP_SRC_RAM
#define PAWN_SPT_MODE				PAWN_SPT_MODE_MSX2

#define MENU_USE_DEFAULT_CALLBACK	FALSE
#define MENU_SCREEN_WIDTH			MENU_VARIABLE
#define MENU_FRAME_X				0
#define MENU_FRAME_Y				6
#define MENU_FRAME_WIDTH			32
#define MENU_FRAME_HEIGHT			8
#define MENU_CHAR_CLEAR				'\0'
#define MENU_CHAR_CURSOR			'@'
#define MENU_CHAR_TRUE				'O'
#define MENU_CHAR_FALSE				'X'
#define MENU_CHAR_LEFT				'<'
#define MENU_CHAR_RIGHT				'>'
#define MENU_TITLE_X				4
#define MENU_TITLE_Y				6
#define MENU_ITEM_X					6
#define MENU_ITEM_Y					8
#define MENU_ITEM_X_GOTO			6
#define MENU_ITEM_ALIGN				MENU_ITEM_ALIGN_LEFT
#define MENU_ITEM_ALIGN_GOTO		MENU_ITEM_ALIGN_LEFT
#define MENU_VALUE_X				14
#define MENU_CURSOR_MODE			MENU_CURSOR_MODE_CHAR
#define MENU_CURSOR_OFFSET			(-2)

#define BITFIELD_MAX				64
#define KANJI_USE_DETECT			FALSE
#define KANJI_USE_LEVEL2			FALSE
#define SCROLL_HORIZONTAL			FALSE
#define SCROLL_VERTICAL				FALSE
#define SCROLL_SRC_X				0
#define SCROLL_SRC_Y				0
#define SCROLL_SRC_W				32
#define SCROLL_SRC_H				24
#define SCROLL_DST_X				0
#define SCROLL_DST_Y				0
#define SCROLL_DST_W				32
#define SCROLL_DST_H				24
#define SCROLL_SCREEN_W				32
#define SCROLL_WRAP					FALSE
#define SCROLL_ADJUST				FALSE
#define SCROLL_ADJUST_SPLIT			FALSE
#define SCROLL_MASK					FALSE
#define SCROLL_MASK_ID				0
#define SCROLL_MASK_COLOR			0
#define SCROLL_MASK_PATTERN			0

#define TILE_WIDTH					16
#define TILE_HEIGHT					16
#define TILE_BPP					8
#define TILE_SCREEN_WIDTH			256
#define TILE_SCREEN_HEIGHT			212
#define TILE_USE_SKIP				TRUE
#define TILE_SKIP_INDEX				0

#define MSXAUDIO_USE_RESUME			FALSE
#define VGM_USE_PSG					TRUE
#define VGM_USE_MSXMUSIC			TRUE
#define VGM_USE_MSXAUDIO			FALSE
#define VGM_USE_SCC					TRUE
#define PCMENC_FREQ					PCMENC_8K
#define PCMPLAY_FREQ				PCMPLAY_8K
#define PCMPLAY_USE_RESTORE			FALSE
#define PT3_SKIP_HEADER				TRUE
#define PT3_AUTOPLAY				FALSE
#define PT3_EXTRA					FALSE
#define TRILO_USE_SFXPLAY			FALSE
#define TRILO_USE_TREMOLO			FALSE
#define TRILO_USE_TAIL				FALSE
#define WYZ_CHANNELS				WYZ_3CH
#define WYZ_USE_DIRECT_ACCESS		FALSE
#define WYZ_CHAN_BUFFER_SIZE		0x20
#define ARKOS_BUFFER_ADDR			0xF000
#define ARKOS_ISR_PROTECTION		FALSE
#define ARKOS_SFX_START_IDX			0
#define ARKOS_USE_EVENT				FALSE
#define NDP_USE_INDERCT				FALSE
#define COMPRESS_USE_RLEP			FALSE
#define COMPRESS_USE_RLEP_DEFAULT	TRUE
#define COMPRESS_USE_RLEP_FIXSIZE	FALSE
#define PLETTER_LENGTHINDATA		FALSE
#define PLETTER_DI_MODE				PLETTER_DI_LOOP
#define PLETTER_WRITE_MODE			PLETTER_WRITE_SAFE
#define BITBUSTER_WRITE_MODE		BITBUSTER_WRITE_SAFE
#define LZ48_MODE					LZ48_MODE_STANDARD
#define MSXi_USE_COMP_NONE			TRUE
#define MSXi_USE_COMP_CROP16		FALSE
#define MSXi_USE_COMP_CROP32		FALSE
#define MSXi_USE_COMP_CROP256		FALSE
#define MSXi_USE_COMP_CROPLINE16	FALSE
#define MSXi_USE_COMP_CROPLINE32	FALSE
#define MSXi_USE_COMP_CROPLINE256	FALSE
#define MSXi_USE_COMP_RLE0			FALSE
#define MSXi_USE_COMP_RLE4			FALSE
#define MSXi_USE_COMP_RLE8			FALSE
#define NTAP_DRIVER					NTAP_DRIVER_MSXGL
#define NTAP_USE_PREVIOUS			FALSE
#define PAC_USE_SIGNATURE			FALSE
#define PAC_USE_VALIDATOR			FALSE
#define PAC_DEVICE_MAX				4
#define PAC_ACCESS					PAC_ACCESS_BIOS
#define QRCODE_VERSION_MIN			1
#define QRCODE_VERSION_MAX			10
#define QRCODE_VERSION_CUSTOM		FALSE
#define QRCODE_USE_BYTE_ONLY		TRUE
#define QRCODE_USE_EXTRA			FALSE
#define QRCODE_BOOST_ECL			FALSE
#define QRCODE_TINY_VERSION			10
#define QRCODE_TINY_ECC				QRCODE_ECC_LOW
#define QRCODE_TINY_MASK			QRCODE_MASK_0

