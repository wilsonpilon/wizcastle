# Changelog — The Wizard's Castle (MSX2+ Deluxe Edition)

All notable changes, specifications, and builds of this project are documented in this file.

## Versioning Scheme (`X.Y`)

- **`X` (Major / Feature Milestone):** Increments whenever a new feature is completed, a new specification is introduced, or a new feature implementation phase begins.
- **`Y` (Minor / Build Increment):** Increments on every compilation/build, regardless of how small the code or asset change is.
- **Releases:** Periodic GitHub Releases (e.g., `v1.1`, `v1.5`, `v2.0`) carry a dedicated release subtitle and consolidate all intermediate changes (`X.1`, `X.2`, `X.3`...) since the previous published release.

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
