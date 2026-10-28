# M4 verification

Status: Windows debug gate passed on 2026-10-28. Linux and GitHub Actions were not run for this milestone.

## Scope

Labeled voxel masks on the loaded CT series. The tools are an HU threshold with a live preview, a seed region grow with 6- or 26-connectivity and an optional axial-slice bound, and a 2D brush that paints or erases on the current slice. Overlays draw on all three MPR planes. Edits undo and redo, and the mask saves and reloads. The current label rebuilds a mesh and records the series it came from.

Not in this milestone: measurements, STL or STEP export, ITK morphology, clinical claims, compressed DICOM, and enhanced multi-frame CT.

## Tests written first

`imaging_label_map_test` failed to link while `LabelMap` was only declared. After the implementation, these cases passed: threshold range, a 6-connected grow that leaves the disconnected component, a diagonal that requires 26-connectivity, grow bounds, another label blocking the grow, a cancelled grow that writes nothing, a brush at a slice edge with 0.8 × 0.5 × 2 mm spacing, undo/redo of the exact voxels, and save/reload with the same voxels, spacing, and origin.

`ui_segmentation_workspace_test` failed to compile until the test included the widget headers the window only forward-declares. `toolSettingsPersist` then restored the brush radius as 1 instead of 2.5, and no settings file was present. `QSettings` was not a reliable writer for that path in one process. The window now writes and reads `layout.ini.segmentation.ini` directly. The restored radius, connectivity, threshold undo/redo/reload, paint versus erase, cancelled grow, and label-mesh provenance then passed.

The first screenshot capture reported that the crosshair grow did not show its banner. The banner text was already `Seed is outside the HU range`; the window had not been shown, so `isVisible()` stayed false. The capture waits on the banner’s own hidden flag. `ui_screenshot_capture_test` then passed.

## Commands

`cmake --build --preset windows-msvc-debug`

`ctest --preset windows-msvc-debug --output-on-failure` — 25/25 passed.

`medforge.exe --capture docs/ui/evidence/M4` at `QT_SCALE_FACTOR` 1 and 1.5, with `QT_QPA_PLATFORM=offscreen`. Offscreen Qt prints `propagateSizeHints` and still writes the PNGs.

## What was inspected

`segment-overlay` at 1280×800, 1920×1080, and 2560×1440, plus `segment-overlay-1280x800-scale150.png`. The crosshair sits on the phantom marker: `i=3 j=2 k=1`, `HU 400`. Axial, sagittal, and coronal each show that voxel as a light overlay. The 80 HU bars stay gray. Properties, scrolled to the segmentation block, read Threshold, Grow, Paint, Erase, with Threshold selected; Label 1 selected and Label 2 unselected; Min 200 HU, Max 1500 HU; Brush 1.0 mm; 6-connected selected; `1 voxel in range`; `Unsaved mask`. Studies shows Segmentation 1. The research line is visible. Tool names are readable at 1280, 1920, 2560, and at 150%. The properties pane scrolls; the volume presets sit above the segmentation block.

`segment-label2-1280x800-scale100.png`: Label 2 is the selected button. The same overlay remains, and Studies still shows Segmentation 1.

`segment-seed-1280x800-scale100.png`: the banner reads Warning, `Seed is outside the HU range`. The status is `Mask saved`. The crosshair is the air voxel `i=4 j=3 k=2`, `HU -1000`.

`segment-mesh` at 1280×800 and 1920×1080, and `segment-mesh-2560x1440-scale150.png`: Studies shows Surface 8 and Segmentation 1. The 3D pane draws a small diamond inside the volume box. Properties read `8 triangles`, and `Label 1 of 1.2.826.0.1.3680043.10.543.100`, with `Unsaved mask`.

The 3D pane in these captures is the software preview. It says `Volume ray casting needs a display`. Ray-cast pixels in the MedForge window were not inspected on a display. Gallery, MPR, and volume PNGs in the same folder were not re-inspected for this milestone.

## Acceptance

| Gate | Status |
| --- | --- |
| Red/green evidence | PASS |
| Windows debug build and 25 tests | PASS |
| Linux build | NOT VERIFIED |
| GitHub Actions | NOT VERIFIED |
| Disconnected component, connectivity, and grow bounds | PASS (`imaging_label_map_test`) |
| Brush at slice edges and nonisotropic spacing | PASS |
| Undo/redo and save/reload of the exact mask | PASS |
| Paint/erase modes, tool persistence, cancel, label mesh provenance | PASS (`ui_segmentation_workspace_test`) |
| Screenshots inspected at 1280, 1920, 2560 and at 150% | PASS for segmentation chrome, overlays, error, and software mesh |
| On-display volume window | NOT VERIFIED |
| No patient data, no clinical claim, no file export | PASS |

## Known limits

ITK was not added. Opening, closing, and smoothing a mask are not implemented.

The brush edits only the slice under the pointer. A radius in millimeters does not paint the neighboring slice. Region grow will not enter a different nonzero label. Cancel drops the partial grow and keeps the previous mask. Threshold overwrites whatever label is already inside the HU range; undo restores it.

The default range is 200–1500 HU, so the phantom’s 80 HU bars are outside it and the 400 HU marker is inside. Preview tints that range without writing the mask. The studies count is every labeled voxel, while mesh extraction uses the selected label. An empty label warns `Label is empty`.

The mask file is `*.mfmask` for this series and these dimensions. A mismatch is rejected. Importing another series clears the mask and the undo stack. Nothing is written to STL or STEP.
