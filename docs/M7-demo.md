# M7 demo — implant pose and a prototype guide

The CT series is the synthetic phantom, not a patient study. The implant and the guide are Open CASCADE solids in millimeters. Nothing here is for manufacture or clinical use.

## Run

Build the debug Open CASCADE 7.9.3 kit once with `tools/build_occt.ps1`. It installs to `third_party/dist/occt-7.9.3-windows-msvc-debug`. Debug builds link that kit. Release builds keep the CAD stub.

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
$env:PATH = "third_party\dist\occt-7.9.3-windows-msvc-debug\win64\vc14\bind;C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
.\build\windows-msvc\bin\Debug\medforge.exe
```

The title bar reads **MedForge M7**. Close a running MedForge before linking, or the linker cannot replace `medforge.exe`.

1. Choose **File → Import CT folder…** and select `tests/fixtures/synthetic-ct`, or any folder written by the synthetic phantom.
2. In Properties, choose **Place cylinder**. A 3 mm diameter, 8 mm tall cylinder is placed just outside the CT bounds. The dock shows the distance, the 0.10 mm tolerance, `No overlap`, and `BRepExtrema`. The 3D pane draws the cylinder and axis lines at its origin. Drag an axis, or edit L, P, S and the three rotations. Rotations are degrees about L, then P, then S, about the world origin, and the translation is applied after that.
3. Move L until the cylinder meets the bounds. The implant turns red and the banner says it intersects the anatomy bounds. Clearance is the minimum B-rep distance. Overlap is the volume of the common solid. The anatomy solid is the axis-aligned bounds of the loaded voxel centers, not a bone surface.
4. Set Offset to 4 mm and Hole to 2 mm. **Preview** builds a 10 × 8 mm contact patch on the +S face, extruded by the offset, with a cylinder cut through it. The dock says `Preview — not applied` and shows the volume. **Apply** keeps it. **Revert** discards it. A non-positive offset, or a hole that does not leave a wall, is refused and the previous guide stays unchanged.
5. Choose **File → Export prototype…** while a guide is applied. The file is STEP, and its header contains `Not for manufacture`. Export is refused during a preview. With no valid B-rep, a triangle mesh is written as binary STL instead, and the banner says it is not an exact STEP. **File → Import implant…** reads STEP or IGES.

## Capture

```powershell
powershell -File tools\capture_screenshots.ps1 -OutputDir docs\ui\evidence\M7
```

Offscreen images show the software volume box, not the ray cast. Inspect `implant-pose` at 1280×800, 1920×1080, and 2560×1440, plus `implant-guide` and `implant-overlap` at 1280×800, at 100% and at 150%.
