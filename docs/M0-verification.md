# M0 verification

Status: local gates pass. GitHub Actions has not been executed, so the remote CI acceptance item is **NOT VERIFIED**. M1 was not started.

## Scope
C++20/Qt 6.12 foundation only.

- CMake presets for MinGW debug and release, CTest, clang-format, and a clang-tidy baseline.
- Module libraries with one-way dependencies: `core`, `imaging`, `geometry`, `cad`, `rendering`, `ui`, `apps/desktop`.
- LPS millimeter voxel conversion, entity ids, cancellation tokens, command history, and a synthetic volume fixture.
- Tokenized stylesheet and developer component gallery (`medforge.exe --gallery`).
- Annotated M1 main-window reference (`medforge.exe --reference`).
- Screenshot capture at 1280×800, 1920×1080, and 2560×1440, at scale factors 1 and 1.5.

Not in this milestone: VTK, DICOM, segmentation, measurements, mesh processing, CAD kernel, docking persistence.

## Assumptions
- The verified Windows compiler is GCC 13.1.0 MinGW because that is the Qt 6.12.0 kit on this machine. MSVC is installed, but no matching Qt MSVC kit is.
- Linux is a supported build and test target. It was verified in WSL2 Ubuntu 26.04.1 with GCC 15.2.0 and Qt 6.12.0 `gcc_64` at `~/Qt/6.12.0/gcc_64`. The default WSL distro is `docker-desktop`, so the build used `wsl -d Ubuntu`. The same sources also passed on Ubuntu 24.04 with GCC 13.3.0.
- Qt stays on the official 6.12.0 installer. `dependencies/versions.lock.json` is the lock. vcpkg is not used. VTK 9.4, ITK 5.4, DCMTK 3.6.9, and Open CASCADE 7.9 are recorded and not fetched.
- No new third-party libraries were added. The JSON sidecar is written by the fixture code.
- Core does not link Qt. UI does not link rendering.
- Primary buttons use dark text on `#4BA3C7` so the label stays readable.
- Screenshots use `QT_QPA_PLATFORM=offscreen` and `QT_SCALE_FACTOR`. That is not a Windows display-scale change and not a physical 2560×1440 panel.

## Tests written first

Stubs returned a zero point with status Ok, ignored the segmented control's disabled state, and accepted every numeric value. Observed failures, from `docs/ui/evidence/M0/tdd-red-*.txt`:

- Coordinates: 7 failed, 2 passed. Identity `(1,2,3)` compared false. Non-positive spacing, non-finite input, and a singular direction were reported as Ok (status 0).
- Segmented control: `disabledFreezesSelection` failed because `setCurrentIndex(2)` returned true while disabled. Right-arrow left the index at 0.
- Numeric editor: out-of-range `12` stayed valid, empty text stayed valid, and a disabled editor stored `9` over the previous value `4`.

Green rerun of those three executables: coordinates 9 passed, segmented control 7 passed, numeric editor 7 passed (`tdd-green-*.txt`). Refactor after green: matrix helpers stay in an anonymous namespace, numeric display goes through `showValue()`, and the little-endian writer uses `std::array`.

Later tests (ids, cancellation, commands, fixtures, banners, progress, dialogs, tokens, screenshot files) were added with their implementations and then run. They are not claimed as separate red runs.

## Commands and results

Toolchain: CMake 4.4.2, Ninja 1.13.2, GCC 13.1.0, Qt 6.12.0 mingw_64.

```text
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug --output-on-failure
```

Debug: **11/11 passed**.

```text
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release --output-on-failure
```

Release, final sources: **11/11 passed**.

Linux, in WSL2 Ubuntu 26.04.1 / GCC 15.2.0 / Qt 6.12.0 `gcc_64`:

```text
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release --output-on-failure
```

Debug and release: **11/11 passed** each. Windows debug and release were reconfigured and retested after the same edits: **11/11 passed** each.

clang-format 20.1.8 `--dry-run --Werror` passed for `src`, `apps`, and `tests`. clang-tidy 20.1.8 was run on `Coordinates.cpp`, `SyntheticVolume.cpp`, and `CommandHistory.cpp` with `.clang-tidy`. No remaining findings in those files under the baseline. Noisy checks (nodiscard on every getter, designated initializers, enum size, math-parentheses on ordinary products) are disabled in that baseline.

## UI evidence

Captures and manifests: `docs/ui/evidence/M0/`. Approved layout baselines:

- `docs/ui/reference/m1-main-window-1280x800.png`
- `docs/ui/reference/m1-main-window-1920x1080.png`
- `docs/ui/reference/m1-main-window-2560x1440.png`

Inspected gallery, reference, foundation, and confirmation dialog images at 100% and 150%. Manifests show device pixel ratio 1.0 and 1.5 (for example 1280×800 logical becomes 1920×1200 pixels at 1.5).

Fixes after inspection:

- Labels no longer paint opaque black chips.
- Disabled segmented buttons stay muted instead of looking selected.
- Icon pixmaps are drawn in Qt 6 logical coordinates, so 150% strokes are not clipped.
- Nested browser rows no longer show a black indent block.
- The confirmation dialog is centered in device pixels at 150%.

At 1280×800 and 1920×1080 the gallery scrolls. Notifications, the empty state, validation, and the delete action are fully visible at 2560×1440, and the scrollbar is present at the smaller sizes. Research notice, units, error text, and the Cancel default on the destructive dialog are readable. No patient data appears.

## Known limitations
- Remote CI has not run. `.github/workflows/ci.yml` is present only.
- MSVC was not built on this machine. The Linux build ran in WSL2 Ubuntu, not on a bare-metal Linux desktop.
- The offscreen platform logs `propagateSizeHints` once per top-level window. It does not fail tests.
- The gallery task is a cancellable worker used to prove progress and cancellation. It does not process images.
- Layout docks in the reference are annotations. They do not persist.

## Design baseline for M1
Keep the `DESIGN_SYSTEM.md` palette, 4 px grid, Segoe UI 10 pt, and Consolas for numbers. Use Fusion plus the generated token stylesheet. Build the shell to the reference images above. Do not add VTK until M1.

## Acceptance checklist

| Gate | Status |
| --- | --- |
| 1. Red-green-refactor evidence for coordinate conversion, disabled segmented control, and numeric validation | PASS |
| 2. Debug and release build, 11/11 tests, on this Windows machine | PASS |
| 2. Debug and release build, 11/11 tests, WSL2 Ubuntu 26.04 GCC 15.2.0 | PASS |
| 2. GitHub Actions result | NOT VERIFIED |
| 3. Qt Test coverage for gallery controls and state | PASS |
| 4. Screenshots captured and inspected at 1280×800, 1920×1080, 2560×1440, scale 1 and 1.5 | PASS |
| 4. Physical monitor and Windows display-scale setting | NOT VERIFIED |
| 5. No dead controls or clipped text in the inspected states | PASS |
| 6. Sample task runs off the GUI thread and can be cancelled | PASS |
| 7. Synthetic fixtures only; research notice visible | PASS |
| 8. README, demo, screenshot workflow, and this report updated | PASS |
| Clean local configure/build/test | PASS |
| CI pass on GitHub | NOT VERIFIED |

Demo: `docs/M0-demo.md`.
