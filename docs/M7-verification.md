# M7 verification

Status: Windows debug gate passed on 2026-10-28. Linux and GitHub Actions were not run for this milestone.

## Scope

Open CASCADE 7.9.3 stays inside `cad`. The public API is boxes, cylinders, a 6-DOF pose, a cut, clearance, STEP and IGES read/write, and a guide built from stored parameters. The workstation places a cylinder or an imported solid on the axis-aligned bounds of the loaded voxel centers, reports distance and overlap with the method and the tolerance, previews the guide, and exports STEP when the solid is a valid B-rep. A triangle mesh exports as STL and is not described as an exact STEP. STEP and STL artifacts say they are not for manufacture.

The local kit is `third_party/dist/occt-7.9.3-windows-msvc-debug`, built by `tools/build_occt.ps1` without Draw or visualization. Debug links `win64/vc14/libd` and loads `win64/vc14/bind`. Release and machines without the kit compile the CAD stub, and the CAD tests skip. `dependencies/versions.lock.json` records 7.9.3. The license is LGPL-2.1.

## Tests written first

`cad_kernel_test` compiled against the stub and skipped every slot because the Debug compile definition was not applied. After that was fixed, `makeBox` reported `Box construction failed`: `BRepPrimAPI_MakeBox::IsDone()` is false until `Shape()` builds the solid. Calling `Shape()` fixed it. The cases then passed: a 10 mm box stays 1000 mm³ after a 5 mm translation and after a 90° rotation about L; two boxes 2 mm apart report that distance at a 0.1 mm deflection and no overlap; an overlapping pair reports a common volume of 500 mm³; a zero tolerance is refused; cutting a box with itself is refused; a zero-size box is refused; a STEP round-trip matches bounds and volume and the file contains `Not for manufacture`; an IGES round-trip matches a 4×3×2 mm box; a text file is rejected as STEP; the same guide parameters repeat to 1e-6 mm³; a hole that does not fit the patch is refused.

`ui_implant_workspace_test` failed to compile with C2027 until the test included the banner type. It then passed: the L-axis tip hits the gizmo, a 12 mm axis measures 12 mm on screen, a 2 mm drag updates L, overlap raises the intersection warning, guide preview blocks export, the reopened STEP matches the guide volume, revert clears it, an invalid offset and an oversized hole are refused, a junk STEP is refused, and a surface with no B-rep exports STL whose header says millimeters and `Not for manufacture`.

## Commands

`powershell -File tools\build_occt.ps1`

`cmake --preset windows-msvc`

`cmake --build --preset windows-msvc-debug`

`ctest --preset windows-msvc-debug --output-on-failure` — 31/31 passed. Preview, Apply, and Revert were then placed on one row so they fit at 1280×800, and the same 31 tests passed again.

`powershell -File tools\capture_screenshots.ps1 -OutputDir docs\ui\evidence\M7` at scale 1 and 1.5. Offscreen Qt prints `propagateSizeHints` and still writes the PNGs. The capture was repeated after the properties row change.

## What was inspected

`implant-pose` at 1280×800, 1920×1080, and 2560×1440, at 100% and at 150%. `implant-guide` and `implant-overlap` at 1280×800, at 100% and at 150%.

The dock reads `Research prototype. Not for manufacture.` The placed cylinder is outside the bounds: `2.500 mm. Tolerance 0.100 mm. No overlap. BRepExtrema.` L is 19.60 mm, P is −20.00 mm, S is 0.00 mm, rotations are 0, tolerance is 0.10 mm, offset is 4.00 mm, and the hole is 2.00 mm. Place cylinder, Preview, Apply, and Revert are fully visible at 1280 and at 150%. The 3D pane shows an orange cylinder, a red L axis, and a blue S axis beside the volume box. The notice banner wraps onto a second line and does not cover the axial image.

`implant-guide` adds a blue contact slab in the 3D pane and the line `Preview — not applied. Volume 307.43 mm³.` That matches a 10×8×4 mm patch minus a 2 mm through-hole.

`implant-overlap` moves L to 11.60 mm. The banner says `Implant intersects the anatomy bounds. Clearance is approximate.` The dock says `0.000 mm. Tolerance 0.100 mm. Overlap. BRepExtrema.` The cylinder is drawn red and meets the blue guide.

The 3D pane says `Volume ray casting needs a display`. Ray-cast pixels and gizmo dragging inside the on-display VTK window were not inspected. Gallery, MPR, volume, segmentation, measurement, and mesh PNGs in the same folder were not re-inspected for this milestone.

## Acceptance

| Gate | Status |
| --- | --- |
| Red/green evidence | PASS |
| Windows debug build and 31 tests | PASS |
| Linux build | NOT VERIFIED |
| GitHub Actions | NOT VERIFIED |
| Box transform keeps volume | PASS |
| Clearance distance, overlap volume, and tolerance refusal | PASS |
| Failed Boolean and invalid size | PASS |
| STEP round-trip, watermark, and rejected file | PASS |
| IGES round-trip | PASS |
| Guide repeatability and hole refusal | PASS |
| Gizmo hit, numeric pose, preview, apply, revert, export | PASS (`ui_implant_workspace_test`) |
| Screenshots inspected at 1280, 1920, 2560 and at 150% | PASS for pose, guide, and overlap |
| On-display volume window | NOT VERIFIED |
| No patient data and no clinical or manufacturing claim | PASS |

## Known limits

Clearance uses the axis-aligned bounds of the loaded voxel centers. It is not a distance to the isosurface.

The guide is that contact patch extruded along +S and cut by a cylinder. It is not a general offset of the anatomy. The same parameters rebuild the same solid. A failed Boolean or a hole that consumes the patch is refused.

Rotations are about the world origin, then the translation. The 3D gizmo translates along L, P, or S. Dragging is tested on the software 3D view. The VTK window draws the same axes; dragging them on a display was not verified.

STEP is written only for a valid B-rep, with `Not for manufacture` in the header. A triangle mesh is binary STL, in millimeters, and the banner says it is not an exact STEP. OBJ and PLY are not written.
