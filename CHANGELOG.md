# Changelog — The Wizard's Castle (MSX2+ Deluxe Edition)

All notable changes, specifications, and builds of this project are documented in this file.

## Versioning Scheme (`X.Y`)

- **`X` (Major / Feature Milestone):** Increments whenever a new feature is completed, a new specification is introduced, or a new feature implementation phase begins.
- **`Y` (Minor / Build Increment):** Increments on every compilation/build, regardless of how small the code or asset change is.
- **Releases:** Periodic GitHub Releases (e.g., `v1.1`, `v1.5`, `v2.0`) carry a dedicated release subtitle and consolidate all intermediate changes (`X.1`, `X.2`, `X.3`...) since the previous published release.

---

## [2.1] - 2026-10-31 — *Phase 2: MSX-DOS 2 Memory Mapper Manager & `WIZDATA.DAT` Asset Pipeline*

### Added
- **Asset Conversion Pipeline (`tools/build_assets.ps1` -> `emul/WIZDATA.DAT`):**
  - Automated extraction and conversion of `winwiz/tiles32.png` and `winwiz/images.png` into a 7-segment (`114,688 bytes` = `112 KB`) MSX2+ asset pack (`WIZDATA.DAT`), bundled automatically onto `DOS2_wizcastle.dsk`:
    - **Segments 0–2 (`48 KB`):** All 40 `32×32` game object sprites (`UNSEEN_ROOM` through `ELF`) in `SCREEN 8` (256-color GRB `3-3-2`), plus `"WIZDATA1"` integrity signature at `0x3F00` in Segment 2.
    - **Segment 3 (`16 KB`):** All 40 `16×16` Dungeon Map tiles (`10 KB` at `0x0000`) and 10 `16×16` Treasure Icons (`RUBY_RED`..`ORB_OF_ZOT`, `2.5 KB` at `0x2800`) in `SCREEN 8` GRB `3-3-2`.
    - **Segments 4–6 (`48 KB`):** All 3 `160×100` splash/event paintings (`TILE_COMBAT`, `TILE_VICTORY`, `TILE_DEATH` from `winwiz/images.png`) encoded in `SCREEN 12` YJK (19,268 colors, 4-pixel YJK packets).
- **MSX-DOS 2 Memory Mapper Manager (`src/wiz_mapper.h`, `src/wiz_mapper.c`):**
  - Allocates 7 user segments (`112 KB`) in the primary Memory Mapper (`DOSMapper_Alloc`) and streams `WIZDATA.DAT` via a safe 2 KB TPA Page 1 buffer (`g_DiskBuf`) with custom `IX`-preserving BDOS 2 file handle routines (`Wiz_DOS2_Open`, `Wiz_DOS2_ReadChunk`, `Wiz_DOS2_Close`) while rendering a graphical progress bar in `SCREEN 7`.
  - Verifies the `"WIZDATA1"` signature in Segment 2 (`0xBF00`) and frees all allocated segments cleanly (`Wiz_MapperCleanup`) on exit to MSX-DOS 2.
  - Provides instant Page 2 (`0x8000–0xBFFF`) bank-switching and VDP transfer routines: `Wiz_MapperDrawYJKPanel()`, `Wiz_MapperUploadTilesToVRAMPage1()`, `Wiz_MapperBlitTile16()`, `Wiz_MapperBlitTreasureIcon16()`, and `Wiz_MapperDrawSprite32()`.
- **Interactive Mapper & Asset Showcase in `[T]` Visual Test (`src/wiz_hwcheck.c`):**
  - In **`SCREEN 12`**, streams the real `160×100` YJK paintings directly from Mapper Segments 4–6 and allows pressing `[1]` (Combat), `[2]` (Victory), or `[3]` (Death) for zero-disk-latency bank switching.
  - In **`SCREEN 8`**, uploads all 40 `16×16` dungeon tiles and 10 treasure icons from Mapper Segment 3 into VRAM Page 1 (`0x10000+`), hardware-blits (`HMMM`) a full `8×8` dungeon floor map and treasure tray, and renders `32×32` sprites (`[1]` Dragon, `[2]` Balrog, `[3]` Chest, `[4]` Elf) from Mapper Segments 0–2 inside the `48×48` Room Viewport.

---

## [1.6] - 2026-10-31 — *Adaptive Graphical Hardware Diagnostic Dashboard (SCREEN 7 / SCREEN 2) & Universal Boot Disk*

> **Release Subtitle:** *Adaptive SCREEN 7 (MSX2/2+) & SCREEN 2 (MSX1) Graphical Hardware Inspector, Universal 3-Layer Boot Disk & Verified SCREEN 12/8 + FM/SCC Test Suite*
> **Consolidated Release (`v1.1` → `v1.6`):** Completes Phase 1 with a full graphical hardware diagnostic dashboard across all MSX generations (`SCREEN 7` 512×212 on MSX2/2+/turbo R and `SCREEN 2` 256×192 on MSX1), a universal boot disk (`MSX-DOS 2` + `MSX-DOS 1` + `MSX Disk-BASIC`), hardware-timed `SCREEN 12`/`SCREEN 8` + `PSG`/`OPLL`/`SCC` verification, and complete updated documentation + multi-PC handoff (`resume.txt`).

### Fixed & Added in `v1.6`
- **`SCREEN 2` & `SCREEN 0 (W40)` Font Table Offset (`src/wiz_hwcheck.c`):**
  - Fixed character glyph offset when loading `g_Font_MGL_Sample6` into `SCREEN 2` (and `SCREEN 0` on MSX 1). Because `g_Font_MGL_Sample6` starts at character `0x00` (offset `4`) rather than `0x20` (`32`), copying from `g_Font_MGL_Sample6 + 4` into tile `32` shifted all displayed characters by `-32` ASCII positions.
  - Updated all `TMS_WriteVRAM` font loads to start at `g_Font_MGL_Sample6 + 4 + (32 * 8)` (offset `260`, `Sprite[32]`), aligning ASCII `32..127` (White), `32..95` (Green at tiles `128..191`), and `32..95` (Red at tiles `192..255`).
- **Complete Documentation & Multi-PC Handoff (`README.md`, `resume.txt`, `docs/`):**
  - Added `resume.txt` with full architectural handoff, technical gotchas, and Phase 2–5 roadmap for seamless continuation on any PC.
  - Added screenshots of the graphical diagnostic on **MSX2+ (`SCREEN 7`)**, **MSX2 (`SCREEN 7`)**, and **MSX1 (`SCREEN 2`)** to `README.md`, and updated `docs/OUTLINE.md`, `docs/MANUAL.md`, and `docs/SPEC.md`.

---

## [1.5] - 2026-10-31 — *Fix MSX1 TMS9918A Register Mirroring & SCREEN 2 Graphical Badges*

### Fixed
- **MSX 1 (`TMS9918A`) Register Mirroring Fix (`src/wiz_hwcheck.c`):**
  - Because `MSXgl`'s `vdp.c` is compiled with `MSX_VERSION=MSX_2P`, calling `VDP_SetMode(VDP_MODE_SCREEN2)` or `VDP_WriteVRAM_16K()` wrote to MSX2/MSX2+ registers `R#8`, `R#9`, `R#10`, `R#11`, `R#13`, `R#14`, and `R#25`.
  - On a real **Texas Instruments `TMS9918A` (MSX 1)**, register indices are masked to 3 bits (`reg & 7`), causing writes to `R#8..R#15` and `R#25` to mirror onto `R#0..R#7`:
    - `R#8` mirrored to `R#0` (`0x00`), clearing the `M3` (`SCREEN 2`) mode bit.
    - `R#9` and `R#25` mirrored to `R#1`, switching the VDP back into `SCREEN 0` (Text 1 mode).
    - `R#10` mirrored to `R#2` (`0x00`), moving the Name Table to `0x0000` so the pattern generator data was displayed as scrambled text.
  - Added dedicated pure-`TMS9918A` helpers (`TMS_RegWrite`, `TMS_FillVRAM`, `TMS_WriteVRAM`) that touch **only** `R#0..R#7` (updating `M_RG0SAV..M_RG7SAV` at `0xF3DF..0xF3E6`), blank the display (`BL=0`) during bulk 16 KB VRAM initialization, and re-enable the display (`BL=1`) once `SCREEN 2` tables are populated.
  - Enhanced the `SCREEN 2` (`256×192`) graphical icons (`g_Sc2IconPatterns` + `g_Sc2IconColors`) to render filled color badges (`[✓]` White on Green, `[✗]` White on Red, `[!]` Black on Yellow, `[i]` White on Blue) and Royal-Blue/Gold horizontal divider bars.

---

## [1.4] - 2026-10-31 — *Universal Boot Disk (MSX-DOS 2 + MSX-DOS 1 + MSX Disk-BASIC Fallback)*

### Added
- **Dual MSX-DOS 1 + MSX-DOS 2 Bootable Disk (`project_config.js`, `emul/dsk/DOS2_wizcastle.dsk`):**
  - Bundled `MSXDOS.SYS` and `COMMAND.COM` (MSX-DOS 1.11) alongside `MSXDOS2.SYS` and `COMMAND2.COM` (MSX-DOS 2) on the same 720 KB `.DSK` image.
  - Because the MSX-DOS boot sector invokes Disk-ROM BDOS `0Fh` (`FOPEN`) to locate `MSXDOS.SYS` when no MSX-DOS 2 ROM is present, any MSX 1 / MSX 2 / MSX 2+ machine with only an MSX-DOS 1 Disk-ROM (`>= 64 KB` RAM) now boots directly into MSX-DOS 1 -> `AUTOEXEC.BAT` -> `WIZCASTLE.COM` and displays the graphical compatibility dashboard (`SCREEN 2` on MSX 1, `SCREEN 7` on MSX 2/2+) instead of dropping to MSX-BASIC.
- **MSX Disk-BASIC Fallback (`emul/AUTOEXEC.BAS`):**
  - Added `AUTOEXEC.BAS` to the disk so that even an MSX 1 with `< 64 KB` RAM (e.g., 16 KB or 32 KB RAM where the Disk-ROM cannot load `MSXDOS.SYS` into Page 0 and falls back to Disk-BASIC) automatically runs `AUTOEXEC.BAS` and displays a formatted compatibility report explaining the missing hardware/OS requirements.
- **Cross-DOS `AUTOEXEC.BAT` & `W40`/`W80` Font Backup (`src/wiz_hwcheck.c`):**
  - Simplified `AUTOEXEC.BAT` to use `REM` instead of MSX-DOS 2-specific internal commands (`COLOR`, `ECHO`), eliminating `Bad command or file name` messages when booting under MSX-DOS 1 `COMMAND.COM`.
  - Updated `Wiz_BackupTextFont()` and `Wiz_RestoreTextMode()` to detect whether `SCREEN 0` started in 40-column (`0x0800`) or 80-column (`0x1000`) mode via `LINL40` (`0xF3AE`) and store the 2,048-byte font backup at `0xD800` in VRAM Page 0 (compatible with both 64 KB VRAM MSX2 and 128 KB VRAM MSX2+ machines).

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
