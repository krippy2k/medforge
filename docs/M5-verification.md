# M5 verification

Status: Windows debug gate passed on 2026-10-28. Linux and GitHub Actions were not run for this milestone.

## Scope

Landmarks in patient LPS millimeters, with stable ids. Euclidean distance (mm), three-point angle (degrees), and the area of a mesh cut by the plane through three landmarks (mm²). Optional snap to the current surface, with an explicit tolerance and a fallback to the clicked point. Overlays on the MPR planes and the 3D pane, a measurement table, and selected-landmark L/P/S editors. Definitions are stored separately from derived values and recomputed when a landmark or the surface changes. CSV and text reports include units, spacing, the series UID, and a research-only disclaimer.

Not in this milestone: STL or STEP export, ITK morphology, clinical claims, compressed DICOM, and enhanced multi-frame CT. No new dependencies. `dependencies/versions.lock.json` is unchanged.

## Tests written first

`geometry_measure_test` failed to link with LNK2019 for `addLandmark`, `addMeasure`, `snapToSurface`, `evaluateMeasures`, `saveMeasureDocument`, `loadMeasureDocument`, `measureReportCsv`, and `measureReportText`. After the implementation, the right-angle case reported 36.8699° because the fixture used a 3-4-5 ray. The fixture was changed to the unit axes. These cases then passed: a 5 mm distance and a 90° angle, a 10×4 box cut of 40 mm², a null mesh reported as not ok, a 90° rotation leaving distance, angle, and area unchanged, snap within 1 mm versus fallback at 5 mm, a save file with no `value=` line that reloads as 5 mm, report text and CSV, and the phantom voxel (3, 2, 1) round-tripping through LPS.

`ui_measurement_workspace_test` failed to compile with C2039 until `WorkspaceWindow` declared the measurement methods. It then passed: Place versus Off, a distance in mm, undo and redo of the table, delete, save/reload of a 3.2 mm span (voxels (0,0,0) to (4,0,0) at 0.8 mm spacing), an L-editor change that recomputes 4.0 mm, a 90° angle from axial clicks, an area that reads `No surface` until the 200 HU isosurface exists and then reports mm², and a report containing the research disclaimer, the uncertainty line, `mm`, and the phantom series UID.

The first `measure-overlay` captures drew no markers. `refreshMeasures` updated the table and left the viewports stale. It now pushes the overlays. The recaptured PNGs show the markers.

## Commands

`cmake --build --preset windows-msvc-debug`

`ctest --preset windows-msvc-debug --output-on-failure` — 27/27 passed, including `geometry_measure_test`, `ui_measurement_workspace_test`, and `ui_screenshot_capture_test`.

`medforge.exe --capture docs/ui/evidence/M5` at `QT_SCALE_FACTOR` 1 and 1.5, with `QT_QPA_PLATFORM=offscreen`. Offscreen Qt prints `propagateSizeHints` and still writes the PNGs.

## What was inspected

`measure-overlay` at 1280×800, 1920×1080, and 2560×1440, at 100% and at 150%.

Axial shows `L2` on the gray 80 HU bar and `L1` on the crosshair, joined by a yellow segment. `L1` is the selected point. Sagittal and coronal, whose slices pass through `L1` only, show that label. The 3D software preview shows a white dot and a blue selected dot beside the volume box. The box is small because the phantom is a few millimeters across.

Properties, scrolled to the measurement notice, read `Research measurements. Not validated. Limited by CT resolution, segmentation, and landmark placement.` Place is the selected tool. Snap is checked at 2.0 mm. The table reads `D1`, `Distance`, `2.600`, `mm`. The Name, Kind, Value, and Unit headers fit. L is 13.20 mm, P is −18.50 mm, and S is 4.00 mm, which is voxel (4, 3, 2) at the phantom origin and spacing. The status line repeats that crosshair and HU −1000. The central banner reads `Research/demo only — not for clinical use.`

At 1920×1080 and 2560×1440, and at 1920×1080 scaled to 150%, Delete landmark, Undo measure, and Redo measure are all visible. At 1280×800, at 100% and at 150%, Redo measure sits just below the properties pane and is reached by scrolling that dock. Tool names, the table, and the landmark chips stay readable at those sizes. The landmark chips use a dark backing so `L1` and `L2` remain readable on the black background and on the gray bar.

The 3D pane says `Volume ray casting needs a display`. Ray-cast pixels in the MedForge window were not inspected on a display. Gallery, MPR, volume, and segmentation PNGs in the same folder were not re-inspected for this milestone.

## Acceptance

| Gate | Status |
| --- | --- |
| Red/green evidence | PASS |
| Windows debug build and 27 tests | PASS |
| Linux build | NOT VERIFIED |
| GitHub Actions | NOT VERIFIED |
| Golden distance, angle, and section area | PASS (`geometry_measure_test`: 5 mm, 90°, 40 mm²) |
| Rigid-transform invariance | PASS |
| Snap tolerance and fallback | PASS |
| Save/reload without stored derived values | PASS |
| 2D click to 3D landmark and LPS round-trip | PASS |
| Tool activation, units, edit, delete, undo/redo, report export | PASS (`ui_measurement_workspace_test`) |
| Area recomputed when the surface appears | PASS |
| Screenshots inspected at 1280, 1920, 2560 and at 150% | PASS for the measurement overlay, table, and units |
| On-display volume window | NOT VERIFIED |
| No patient data, no clinical claim, no mesh file export | PASS |

## Known limits

The angle vertex is the second of the three clicks. Distance and area use the two or three nearest landmarks within 8 mm of the crosshair. Switching tools clears a partial pick.

Section area is the sum of closed mesh–plane loops. It needs the current surface. An open loop is reported as `Open cross-section` and is not given an area. Snap uses that same surface.

The measurement file is `MFMEASURE 1` for one series. A series mismatch is rejected. Importing another series clears landmarks and the measurement undo stack. Edit → Undo does not undo a measurement. Nothing is written to STL or STEP.

The numbers are not a validated measurement. They are limited by voxel spacing, the segmentation or isosurface, and where the landmark was placed.
