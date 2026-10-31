// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  The Wizard's Castle - MSX2+ Deluxe Edition
//  Project Configuration
//─────────────────────────────────────────────────────────────────────────────

//*******************************************************************************
// BUILD STEPS
//*******************************************************************************

DoClean   = false;
DoCompile = true;
DoMake    = true;
DoPackage = true;
DoDeploy  = true;
DoRun     = false;

//*****************************************************************************
// PROJECT SETTINGS
//*****************************************************************************

//-- Project name (string). Will be use for output filename
ProjName = "wizcastle";

//-- List of project modules to build (array). If empty, ProjName will be added
ProjModules = [
	"wizcastle",
	"src/wiz_hwcheck"
];

//-- List of library modules to build (array)
LibModules = [
	"system",
	"bios",
	"dos",
	"dos_mapper",
	"vdp",
	"print",
	"draw",
	"input",
	"memory",
	"string",
	"math",
	"psg",
	"msx-music",
	"scc"
];

//-- Target MSX machine version (string)
//   - 2P       MSX2+
Machine = "2P";

//-- Target program format (string)
//   - DOS2     .com    MSX-DOS 2 program (starting at 0100h)
Target = "DOS2";

//-- Disable automatic ROM version check so our custom Hardware Diagnostic Stub can run on any MSX
CheckVersion = false;

//-- Parse MSX-DOS command-line arguments
DOSParseArg = false;

//-- Size of the final disk (.DSK file). Can be "360K" or "720K" (string)
DiskSize = "720K";

//*******************************************************************************
// SIGNATURE SETTINGS
//*******************************************************************************

AppSignature = true;
AppCompany = "WC";
AppID = "2P";

//*******************************************************************************
// MAKE SETTINGS
//*******************************************************************************

Optim = "Speed";
CompileComplexity = "Default";
CompileSkipOld = true;
Verbose = true;

//*******************************************************************************
// EMULATOR SETTINGS
//*******************************************************************************

EmulMachine    = true;
Emul60Hz       = true;
EmulFullScreen = false;
EmulMute       = false;
EmulDebug      = false;
EmulTurbo      = false;

//-- Extension options
EmulSCC        = true;
EmulMSXMusic   = true;
EmulRAM        = true;
