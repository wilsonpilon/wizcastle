# Changelog — The Wizard's Castle (MSX2+ Deluxe Edition)

All notable changes, specifications, and builds of this project are documented in this file.

## Versioning Scheme (`X.Y`)

- **`X` (Major / Feature Milestone):** Increments whenever a new feature is completed, a new specification is introduced, or a new feature implementation phase begins.
- **`Y` (Minor / Build Increment):** Increments on every compilation/build, regardless of how small the code or asset change is.
- **Releases:** Periodic GitHub Releases (e.g., `v1.1`, `v1.5`, `v2.0`) carry a dedicated release subtitle and consolidate all intermediate changes (`X.1`, `X.2`, `X.3`...) since the previous published release.

---

## [1.3] - 2026-10-31 — *Graphical Hardware Diagnostic Dashboard (SCREEN 7 on MSX2/2+ & SCREEN 2 on MSX1)*

### Added
- **`SCREEN 7` High-Resolution Graphical Diagnostic Dashboard (`src/wiz_hwcheck.c`):**
  - Automatically rendered in **512×212, 16 colors (`VDP_MODE_SCREEN7`)** when launched on **MSX 2**, **MSX 2+**, or **MSX turbo R** (`V9938`/`V9958`).
  - Features a royal-blue and gold header banner, 7 alternating slate/black inspection cards, and custom **20×12 pixel graphical status badges**:
    - **Green Checkmark (`[✓]`):** Requirement satisfied (`PASS`).
    - **Red Cross (`[✗]`):** Critical requirement missing (`FAIL`).
    - **Yellow Exclamation (`[!]`):** Recommended feature fallback active (`WARN`).
    - **Cyan Info (`[i]`):** Optional enhancement not detected (`INFO`).
  - Color-coded concise labels and hardware metrics (`MSX-DOS`, `MACHINE`, `VIDEO`, `MAPPER`, `PSG`, `FM OPLL`, `SCC`) with a full-width verdict banner (`READY TO PLAY`, `COMPATIBLE (SCREEN 8)`, or `INCOMPATIBLE HARDWARE`) and interactive command footer (`[T] Video/Sound Test`, `[S] Sound Only`, `[Q/ESC] Exit to DOS`).
- **`SCREEN 2` MSX 1 Graphical Diagnostic Fallback (`src/wiz_hwcheck.c`):**
  - Automatically rendered in **256×192 (`VDP_MODE_SCREEN2`, 32×24 tiles)** if launched on an **MSX 1 (`TMS9918A`)**.
  - Uploads custom 8×8 tile patterns (`0x80` = Green Checkmark `✓`, `0x81` = Red Cross `✗`, `0x82` = Yellow Exclamation `!`, `0x83` = Solid Frame Bar) across all three `SCREEN 2` pattern/color banks with per-row color attributes.
- **Early `SCREEN 0` Text Font Preservation (`Wiz_BackupTextFont` / `Wiz_RestoreTextMode`):**
  - Saves the 2,048-byte `SCREEN 0` font from VRAM `0x1000` into upper VRAM (`0x1F000`, Page 1 bottom) before switching to `SCREEN 7`, and cleanly restores `SCREEN 0` (80 columns) when exiting back to MSX-DOS.

---

## [1.2] - 2026-10-31 — *Fix SCREEN 12 / SCREEN 8 Visual & Audio Test and VRAM Font Restore*

### Fixed
- **`SCREEN 12` Text & Crash Fix (`src/wiz_hwcheck.c`):** Fixed a critical issue where `VDP_SetMode(VDP_MODE_SCREEN12)` set `g_VDP_Data.Mode = 12` (`VDP_MODE_SCREEN12`), which is not handled by `MSXgl`'s `Print_SetMode()` / `Print_Initialize()` `switch` block (only `VDP_MODE_GRAPHIC7` = `8` was handled), leaving `PRINT_DATA.DrawChar = NULL (0x0000)`. Calling `Print_DrawText()` in `SCREEN 12` previously jumped to `0000h` (MSX-DOS Warm Boot) while still in `SCREEN 12`, causing MSX-DOS to print prompt characters as colored dashes at the top of VRAM and aborting before audio or `SCREEN 8` could run.
- **Hardware-Timed `WaitFrames()` (`src/wiz_hwcheck.c`):** Replaced `Halt()` with direct polling of V9958 Status Register `S#2` bit 6 (`VR` Vertical Retrace) with safety timeouts, making frame timing 100% immune to `DI`/`EI` interrupt state changes caused by `RDSLT`.
- **MSX-MUSIC (`YM2413` OPLL) Hardware Timing & Port Enable (`src/wiz_hwcheck.c`):** Added `OPLL_WriteReg()` with mandatory hardware wait cycles after index (`7Ch`) and data (`7Dh`) writes, and enabled `7FF6h` bit 0 for internal as well as external MSX-MUSIC slots.
- **Clean `SCREEN 0` (Width 80) VRAM Font Backup & Restore (`src/wiz_hwcheck.c`):** Backed up the 2,048-byte MSX-DOS 2 text font from VRAM Page 0 (`0x1000`) into VRAM Page 1 (`0x18000`) prior to the `SCREEN 12`/`SCREEN 8` visual tests and restored it upon returning to `SCREEN 0` (Width 80), preventing VRAM font corruption.
- **Non-Echoing Console Input (`Wiz_GetKeyNoEcho`):** Switched interactive key prompts to BDOS `07h` (`DOS_FUNC_DIRIN`) so key presses are never echoed into bitmap VRAM.

---

## [1.1] - 2026-10-31 — *Foundation, Technical Specification & MSX2+ Hardware Diagnostic Stub*

> **Release Subtitle:** *Project Architecture, Complete Documentation Suite & Interactive Hardware Inspector*
> **Baseline Release:** Initial milestone establishing the complete engineering specification, bilingual player/developer documentation, MSXgl build setup, and the interactive MSX-DOS 2 hardware compatibility diagnostic program (`WIZCASTLE.COM`).

### Added
- **Documentation Suite (`docs/`):**
  - [`docs/PLAN.md`](docs/PLAN.md): Complete 5-phase engineering and architectural implementation plan.
  - [`docs/SPEC.md`](docs/SPEC.md): Technical specification detailing MSX2+ hardware validation, Z80 64 KB TPA + 1024 KB Memory Mapper segmentation, V9958 VRAM layout (`SCREEN 12` YJK & `SCREEN 8` GRB + Page 1 tile cache), lVGM + ayFX audio architecture (`PSG` + `MSX-MUSIC OPLL` + `Konami SCC`), and 100% compatible hybrid gameplay rules (`WinWiz` C++ mode vs. `Classic 1980` BASIC mode).
  - [`docs/OUTLINE.md`](docs/OUTLINE.md): Master roadmap tracking all original features, MSX2+ visual/audio enhancements, completed items, and upcoming phases.
  - [`docs/MANUAL.md`](docs/MANUAL.md): Complete guide divided into Part I (Toolchain, compilation, emulator setup, and hardware diagnostics) and Part II (Complete Player's Manual covering races, stats, equipment, spells, treasures, monsters, vendors, and commands).
- **Repository & Licensing (`LICENSE`, `README.md`, `CHANGELOG.md`):**
  - Dual licensing setup (**CC BY-SA 4.0** for combined distribution with MSXgl; **MIT + CC BY-SA 4.0** for standalone C port code in `MSXgl/projects/wizcastle/` and `docs/`), with full attribution to Joseph R. Power (1980), Daniel D. Miller (`winwiz`), Guillaume "Aoineko" Blanchard (`MSXgl`), and the NetHack DevTeam.
- **MSXgl Project Structure (`MSXgl/projects/wizcastle/`):**
  - [`project_config.js`](MSXgl/projects/wizcastle/project_config.js) & [`msxgl_config.h`](MSXgl/projects/wizcastle/msxgl_config.h): Configured for `Machine = "2P"` (MSX2+) and `Target = "DOS2"` (`0100h` `.COM` executable with automatic bootable `.DSK` packaging and inter-slot BIOS calls from RAM in Page 0).
  - [`src/wiz_version.h`](MSXgl/projects/wizcastle/src/wiz_version.h): Centralized `X.Y` version tracking (`v1.1`).
- **Hardware Detection & Compatibility Stub (`src/wiz_hwcheck.c`, `src/wiz_hwcheck.h`, `wizcastle.c`):**
  - Itemized startup inspector checking:
    1. **OS Environment:** MSX-DOS 2 (`>= 2.20`) / Nextor vs. MSX-DOS 1.x, plus TPA upper address (`0006h`).
    2. **MSX Generation:** MSX1 / MSX2 / MSX2+ / MSX turbo R (`002Dh`) and VDP refresh rate (`50Hz PAL` / `60Hz NTSC`).
    3. **Video & VRAM:** TMS9918A / Yamaha V9938 / Yamaha V9958 detection and VRAM capacity (`16` / `64` / `128+` KB).
    4. **Memory Mapper:** Safe `HOKVLD` Extended BIOS check (`FFCAh`) and MSX-DOS 2 Mapper table inspection reporting slot ID, total RAM (`1024 KB` recommended, `256 KB` minimum), and free 16 KB segments (`>= 8` segments / `128 KB` free required).
    5. **PSG Audio:** Internal AY-3-8910 / YM2149 loopback verification.
    6. **FM Audio:** MSX-MUSIC (YM2413 / OPLL) slot search (`FMPAC` / `OPLL` signature).
    7. **SCC Audio:** Konami SCC / SCC+ cartridge slot auto-detection.
  - **Interactive Visual & Audio Hardware Test (`[T]` key):**
    - Tests `SCREEN 12` (19,268 YJK color gradient bars).
    - Tests `SCREEN 8` (256 GRB color palette matrix + VDP text output).
    - Plays simultaneous C-Major arpeggio chord across **PSG**, **MSX-MUSIC (FM)**, and **Konami SCC**.
- **Compiled Binaries (`v1.1`):**
  - [`MSXgl/projects/wizcastle/out/wizcastle.com`](MSXgl/projects/wizcastle/out/wizcastle.com) (`18,895 bytes`, `0100h–4ACEh`).
  - [`MSXgl/projects/wizcastle/emul/dsk/DOS2_wizcastle.dsk`](MSXgl/projects/wizcastle/emul/dsk/DOS2_wizcastle.dsk) (Bootable 720 KB MSX-DOS 2 floppy image).
