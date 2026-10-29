# M6 verification

Status: Windows debug gate passed on 2026-10-28. Linux and GitHub Actions were not run for this milestone.

## Scope

Mesh topology in patient millimeters: bounds, surface area, volume when watertight, component count, open edges, and non-manifold edges. Laplacian smoothing, shortest-edge decimation, and bounded hole fill. Each one is a preview until Apply repair. Undo and redo restore the committed mesh. Cancel drops an in-progress repair. Binary STL export and reimport, with millimeters labeled in the dock, the banner, and the file header.

Not in this milestone: STEP, OBJ, PLY, Open CASCADE, CGAL, ITK morphology, and clinical claims. No new dependencies. `dependencies/versions.lock.json` is unchanged. Repair stays in `geometry` and does not call VTK.

## Tests written first

`geometry_mesh_process_test` failed to link with LNK2019 for `inspectMesh`, `smoothMesh`, `decimateMesh`, `fillSmallHoles`, `writeBinaryStl`, `readBinaryStl`, and `stlUnitLabel`. After the implementation these cases passed: a 10 mm cube has area 600 mm², volume 1000 mm³, one component, and no open or non-manifold edges; a removed face is not watertight and has volume 0; two separate triangles are two components; three triangles on one edge are non-manifold; a 4-edge hole is refused at a limit of 3 and filled at a limit of 8 back to 1000 mm³; one smoothing step moves a cube corner and leaves the input unchanged; decimation of a radius-10 sphere reduces the triangle count and stays inside the original bounds; a cancelled token returns the input; a binary STL round-trips the cube, and a truncated file is rejected.

`ui_mesh_workspace_test` failed to compile with C2039 until `WorkspaceWindow` declared the repair methods. The export case then failed because `errorBanner()->isVisible()` stays false until the window is shown, and a second preview was started while the first repair thread was still running. The test now reads the banner text and waits for the first repair to finish. Preview, cancel, apply, undo, redo, a refused or empty fill that does not change the committed triangle count, export blocked during preview, STL header millimeters, and a cancelled repair then passed.

## Commands

`cmake --build --preset windows-msvc-debug`

`ctest --preset windows-msvc-debug --output-on-failure` — 29/29 passed. After the diagnostics label was shortened so Redo mesh fits at 1280×800, `ui_mesh_workspace_test` was run again and passed.

`medforge.exe --capture docs/ui/evidence/M6` at `QT_SCALE_FACTOR` 1 and 1.5, with `QT_QPA_PLATFORM=offscreen`. Offscreen Qt prints `propagateSizeHints` and still writes the PNGs. The capture was repeated after the label change.

## What was inspected

`mesh-diagnostics` at 1280×800, 1920×1080, and 2560×1440, at 100% and at 150%. `mesh-preview` and `mesh-export` at 1280×800, at 100% and at 150%.

The phantom surface at 0 HU is 118 triangles. Studies shows Surface 118. Diagnostics read `Not watertight`, `Area 3.018 mm²`, `Volume unavailable`, `5 components, 42 open edges, 0 non-manifold edges`, and `STL units: millimeters`. The note says the preview does not replace the source mesh. Smooth is selected. Iterations is 1, Keep is 50%, and Hole is 12 edges. Preview repair, Apply repair, Cancel preview, Undo mesh, and Redo mesh are fully visible at 1280 and at 150%. The 3D software preview shows a small diamond inside the volume box. The central banner reads `Research/demo only — not for clinical use.`

`mesh-preview` adds `Preview — not applied` above the same source numbers. The 3D pane shows the smoothed candidate as a few specks instead of the diamond. Studies still shows 118.

`mesh-export` shows the notice `Exported 118 triangles. STL units are millimeters. Series 1.2.826.0.1.3680043.10.543.100`. The notice sits under the research line and does not cover the axial image. The source diagnostics are unchanged.

The 3D pane says `Volume ray casting needs a display`. Ray-cast pixels in the MedForge window were not inspected on a display. Gallery, MPR, volume, segmentation, and measurement PNGs in the same folder were not re-inspected for this milestone.

## Acceptance

| Gate | Status |
| --- | --- |
| Red/green evidence | PASS |
| Windows debug build and 29 tests | PASS |
| Linux build | NOT VERIFIED |
| GitHub Actions | NOT VERIFIED |
| Cube area and volume | PASS (600 mm², 1000 mm³) |
| Open, non-manifold, and component counts | PASS |
| Small-hole fill and refusal of a larger hole | PASS |
| Smoothing does not change the input mesh | PASS |
| Decimation stays inside the source bounds | PASS |
| Cancellation | PASS |
| STL export/reimport of bounds and volume | PASS |
| Preview, apply, cancel, undo/redo, export blocked during preview | PASS (`ui_mesh_workspace_test`) |
| Screenshots inspected at 1280, 1920, 2560 and at 150% | PASS for diagnostics, preview, and export feedback |
| On-display volume window | NOT VERIFIED |
| No patient data, no clinical claim, no STEP export | PASS |

## Known limits

Volume is reported only when every edge is used by exactly two triangles. An open or non-manifold mesh reports `Volume unavailable`.

Smoothing pins vertices on an open boundary. Decimation collapses the shortest edge to its midpoint and can drop more triangles than the keep ratio in one step. It does not promise a watertight result.

Hole fill triangulates a closed boundary with a fan from the first vertex. A hole with more edges than the limit, or a boundary that does not close, is refused and the source is not changed. Filling does not invent a surface across a large opening.

The STL writer emits binary triangles in millimeters. The 80-byte header says so. Reimport welds vertices that have identical float coordinates and compares bounds within 0.001 mm. OBJ and PLY are not written. STEP is not written.

A new surface, or a new CT series, clears the mesh undo stack. Edit → Undo does not undo a mesh repair.
