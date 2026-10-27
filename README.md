# MedForge — CT-to-CAD Research Workstation

A greenfield C++20/Qt 6 desktop application for CT visualization, segmentation, mesh processing, anatomical measurements, and experimental CAD planning. **Research and demonstration only; not for clinical use.**

M0 establishes the build, module boundaries, design tokens, and developer component gallery. DICOM import, viewports, segmentation, measurements, and CAD are later milestones.

## Milestone sequence
M0 Foundation → M1 Desktop shell → M2 DICOM/MPR → M3 Volume/mesh → M4 Segmentation → M5 Measurements → M6 Mesh repair/export → M7 Guide/implant planning → M8 Validation/release.

**Interview checkpoint:** M0–M5. M6 adds substantial CAD credibility; M7 is an advanced stretch milestone.

## Architecture
Dependency direction is one-way:

`core` ← `imaging`, `geometry` ← `cad`, `core` ← `rendering`, `core` ← `ui` ← `apps/desktop`

- `core` has no Qt dependency. It owns LPS millimeter conversion, entity ids, cancellation tokens, command history, and the synthetic volume fixture.
- `imaging`, `geometry`, `cad`, and `rendering` are linked boundaries only. VTK, DICOM, segmentation, and CAD kernels are not compiled in M0.
- `ui` owns tokens, the generated stylesheet, shared widgets, the component gallery, and the M1 layout reference.
- Patient coordinates are LPS millimeters. Voxel conversion is `origin + direction × (index ∘ spacing)`. Spacing must be positive and the direction matrix must be invertible.

## Toolchain
Pinned in `dependencies/versions.lock.json`.

- C++20, CMake 3.27+, Ninja
- Qt 6.12.0 Widgets and Test
- Windows: GCC 13.1.0 MinGW (`C:\Qt\Tools\mingw1310_64`) with Qt `mingw_64`
- Linux: GCC with the Qt `gcc_64` kit. On this machine that is WSL2 Ubuntu 26.04, GCC 15.2.0, with Qt at `~/Qt/6.12.0/gcc_64`. The presets also look in `/opt/Qt/6.12.0/gcc_64`, or use `CMAKE_PREFIX_PATH`. Run them with `wsl -d Ubuntu`, because the default WSL distro is `docker-desktop`.
- clang-format 20.1.8

An MSVC Qt kit is not installed on the reference machine, so the verified Windows build is MinGW. Windows remains the primary UI target. Linux configure, build, and test were verified in WSL2 Ubuntu 26.04 with GCC 15.2.0 and Qt 6.12.0 `gcc_64`. The Linux CI job targets Ubuntu 24.04 and lists the system packages that kit needs (GLib, fontconfig, and the XCB/OpenGL libraries). The official Qt kit carries its own ICU 73 libraries.

## Development commands

Windows:

```powershell
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug --output-on-failure

cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release --output-on-failure
```

Linux:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure

cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release --output-on-failure
```

The desktop executable is `build/<preset>/bin/medforge` (`.exe` on Windows).

```text
medforge --gallery
medforge --reference
medforge --capture docs/ui/evidence/M0
```

Screenshot steps are in `docs/ui/screenshot-workflow.md`. A short demo script is in `docs/M0-demo.md`.

Generate the checked-in synthetic sphere fixture with:

```text
medforge-fixtures --output tests/fixtures/synthetic/sphere8 --size 8 --seed 1
```

The fixture is synthetic. It is not patient data.

## Design baseline
Tokens live in code and are written into the Qt stylesheet. The approved M1 wireframe is `docs/ui/reference/m1-main-window-1920x1080.png` (with 1280×800 and 2560×1440 siblings). M1 should reuse these tokens and components.

## Limitations
- No DICOM reader, viewport, segmentation, measurement, mesh, or CAD kernel.
- The gallery sample task is a cancellable progress demonstration, not an imaging operation.
- Remote GitHub Actions has not been executed from this workspace. The Linux job in `.github/workflows/ci.yml` matches the Ubuntu packages used for the local container build.
- Visual checks use Qt offscreen capture and `QT_SCALE_FACTOR`, not a physical 2560×1440 panel or a Windows 150% display setting.
- Not a medical device. Do not use for diagnosis or treatment.

## Working agreement for coding agents
- Implement only the active milestone; do not silently expand scope.
- Write a brief implementation plan and list assumptions before editing.
- Add deterministic automated tests for all core math and data transformations.
- Keep core algorithms independent of Qt widgets and graphics backends.
- Prefer established, appropriately licensed libraries over custom DICOM parsers, CAD kernels, and numerical algorithms.
- Require explicit approval for new major dependencies, architecture changes, or destructive file operations.
- Every milestone ends with a reproducible build, test results, and a short demonstration script.
- No patient-identifying data in repository, logs, screenshots, or test fixtures.

## Definition of done (all milestones)
1. Fresh configure/build succeeds on supported development platform.
2. Automated tests pass, with regression tests for defects fixed.
3. Main-thread UI remains responsive during long tasks.
4. Failure states are reported clearly; no silent data corruption.
5. README and milestone-specific user/developer documentation updated.
6. Screenshots or a short recorded demo show the completed workflow.

## Cursor execution and design gates
- Permanent rules live in `.cursor/rules/` (`development.mdc`, `ui-design.mdc`, `autonomy.mdc`).
- Read `DESIGN_SYSTEM.md` and `QUALITY_GATES.md` before every milestone.
- M0 must deliver the component gallery, design tokens, main-window reference, screenshot inspection workflow, and red-green-refactor evidence before M1.
- Cursor project rules do not override Cursor's actual terminal permission controls.

License inventory: `third_party/LICENSES.md`.
