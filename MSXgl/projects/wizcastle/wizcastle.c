// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Main Entry Point & Graphical Hardware Diagnostic Inspector
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

	// 2. Render Graphical Diagnostic Screen:
	//    - SCREEN 2 (256x192) on MSX1
	//    - SCREEN 7 (512x212) on MSX2 / MSX2+ / MSX turbo R
	Wiz_ShowGraphicalDiagnostic();

	// 3. If incompatible, beep, wait for any key, restore text mode, and exit
	if (!g_HwInfo.isCompatible)
	{
		DOS_Beep();
		Wiz_GetKeyNoEcho();
		Wiz_RestoreTextMode();
		DOS_StringOutput("The Wizard's Castle: Incompatible hardware detected. Exiting to DOS.\r\n$");
		return 1;
	}

	// 4. Play confirmation chord on detected chips (PSG + FM + SCC)
	Wiz_PlayTestChord();

	// 5. Interactive loop on Graphical Diagnostic Dashboard
	while (1)
	{
		c8 key = Wiz_GetKeyNoEcho();
		if (key == 't' || key == 'T')
		{
			Wiz_RunHardwareVisualTest();
			Wiz_ShowGraphicalDiagnostic();
		}
		else if (key == 's' || key == 'S')
		{
			Wiz_PlayTestChord();
		}
		else if (key == 'q' || key == 'Q' || key == 0x1B)
		{
			break;
		}
	}

	// 6. Cleanly restore MSX-DOS 80-column text mode & font before exiting
	Wiz_RestoreTextMode();
	DOS_StringOutput("The Wizard's Castle MSX2+ (v" WIZ_VERSION_STR ") - Returning to MSX-DOS 2...\r\n$");
	return 0;
}
