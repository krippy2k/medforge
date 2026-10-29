# M8 verification — project file, benchmark, unsigned package

Scope: versioned `MFPROJECT 1` files with atomic replace, checksum refusal, and sidecar mask and measurements. A phantom benchmark records import, threshold segmentation, isosurface, and STL export. The Windows folder is an unsigned research package. No new imaging or CAD feature.

## Tests first

`core_project_test` failed to compile with C1083: `medforge/core/Project.h` did not exist.

After the writer and loader existed, `fingerprintDetectsAChangedVoxel` failed because the sample document still carried the phantom series UID while the fingerprint used `1.2.3`. The test was corrected to that UID. It then passed. The production comparison still requires the UID and dimensions to match.

`ui_project_workspace_test` failed to compile with C2039 (`saveProject` was not a member of `WorkspaceWindow`) and C2027 (`Banner` was incomplete). The test includes `FoundationWidgets.h`.

The first keyboard check failed: `QLineEdit::hasFocus()` was false before the window was activated. `activateWindow()` plus `Qt::StrongFocus` then passed, and the focus widget was the window editor.

`core_benchmark_test` failed to compile when clang-format ordered `psapi.h` ahead of `windows.h`. The Windows headers are wrapped in `clang-format off`. The test then passed.

## Commands and results

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug --output-on-failure
```

34/34 passed. `ui_project_workspace_test` was 1.09 s and `ui_screenshot_capture_test` was 14.34 s. `cad_kernel_test` passed with the local Open CASCADE debug kit.

Direct benchmark run, debug, synthetic 8×6×4 phantom, threshold 0 HU:

| Step | Time | Result |
| --- | --- | --- |
| Import | 0 ms | series accepted |
| Threshold segmentation | 0 ms | voxel (3,2,1) label 1 |
| Isosurface | 2 ms | 118 triangles |
| Binary STL export | 0 ms | triangle count matches on read |
| Working set | 26,910,720 bytes | under the 2 GB guard |

Times of 0 ms are below the 1 ms clock. The test still fails if a step exceeds 15 s, or 30 s for the isosurface. This is a hang guard on the phantom, not a clinical performance claim.

`powershell -File tools/package_unsigned.ps1` wrote `build/medforge-0.1.0-windows-unsigned`. The folder contains `medforge.exe`, `medforge_fixtures.exe`, Qt and VTK runtime files, `platforms`, `UNSIGNED.txt`, and `RELEASE.txt`. It does not contain the unit-test executables. `UNSIGNED.txt` says the build is unsigned and not for clinical use. This was a copy on the development machine, not a clean VM install.

## UI evidence

Captured with `tools/capture_screenshots.ps1 -OutputDir docs/ui/evidence/M8` at `QT_SCALE_FACTOR` 1 and 1.5. Offscreen Qt prints `propagateSizeHints` and the 3D pane says volume ray casting needs a display.

Inspected `project-saved` at 1280×800, 1920×1080, and 2560×1440, at 100%, and 1280×800 at 150%:

- The research banner and `Project saved. Not for clinical use.` are each one readable line and do not cover the axial image.
- Axial, sagittal, and coronal stay visible. The sagittal view shows the L2 landmark. The status bar shows `i=4 j=3 k=2`, LPS millimeters, and HU −1000.
- At 1280 and at 150%, the toolbar and the threshold, Build surface, and camera buttons are fully visible. The properties dock still scrolls to the implant fields. At 2560 those fields are on screen.
- Text is not clipped in the project notice. Contrast matches the existing dark theme.

## Gates

| Gate | Status |
| --- | --- |
| Red-green evidence for project save, load, checksum, and missing series | PASS |
| Windows debug build and 34/34 ctest | PASS |
| Qt Test for save, reopen, export, retry, checksum, worker join, shortcuts, and focus | PASS |
| Screenshots inspected at 1280, 1920, 2560 and at 150% for the project state | PASS |
| No dead project actions; notice not clipped | PASS |
| Cancellation already covered; destructor joins an in-flight import | PASS |
| Synthetic phantom only; research notice in the app, project file, and package | PASS |
| Linux, GitHub Actions, on-display ray cast, on-display gizmo drag | NOT VERIFIED |
| AddressSanitizer on the full app (`MEDFORGE_ENABLE_ASAN` is off; VTK and Open CASCADE are unsanitized) | NOT VERIFIED |
| Clean-machine install | NOT VERIFIED |

## Known limits

The project checksum is a little-endian fingerprint of the series UID, dimensions, and HU samples. It is not a portable archive of the voxels. Save writes the mask and measurements, then replaces the manifest via a temporary file and a backup. A crash between those sidecars and the manifest can leave sidecars without a new manifest. Rotations and the guide are restored only when Open CASCADE is linked. The unsigned package is a debug folder, not an installer.
