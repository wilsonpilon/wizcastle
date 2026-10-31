// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Main Entry Point & Hardware Diagnostic Stub
//─────────────────────────────────────────────────────────────────────────────

#include "msxgl.h"
#include "dos.h"
#include "src/wiz_hwcheck.h"

//=============================================================================
// MAIN ENTRY POINT
//=============================================================================

int main(char** argv, int argc)
{
	(void)argv;
	(void)argc;

	// 1. Run full hardware & OS detection
	Wiz_DetectHardware();

	// 2. Display itemized compatibility report on MSX-DOS console
	Wiz_PrintHardwareReport();

	// 3. Handle result
	if (!g_HwInfo.isCompatible)
	{
		DOS_Beep();
		DOS_StringOutput("\r\nPress any key to return to MSX-DOS...$");
		Wiz_GetKeyNoEcho();
		return 1;
	}

	// Play confirmation chord on detected chips (PSG + FM + SCC)
	Wiz_PlayTestChord();

	DOS_StringOutput("\r\nOptions: [T] Visual & Sound Hardware Test   [Q / ESC] Exit to DOS\r\n$");

	while (1)
	{
		c8 key = Wiz_GetKeyNoEcho();
		if (key == 't' || key == 'T')
		{
			Wiz_RunHardwareVisualTest();
			Wiz_PrintHardwareReport();
			DOS_StringOutput("\r\nOptions: [T] Visual & Sound Hardware Test   [Q / ESC] Exit to DOS\r\n$");
		}
		else if (key == 'q' || key == 'Q' || key == 0x1B)
		{
			break;
		}
	}

	DOS_StringOutput("\r\nReturning to MSX-DOS 2...\r\n$");
	return 0;
}
