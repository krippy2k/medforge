# M5 demo — measurements

The series in `tests/fixtures/synthetic-ct` is generated, not a patient study. Four axial slices, 8×6 pixels. Spacing is 0.8 × 0.5 × 2 mm. Air is −1000 HU. Slice `k` paints column `i = k` at 80 HU. Pixel (3, 2) on slice 1 is 400 HU.

## Run

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
.\build\windows-msvc\bin\Debug\medforge.exe
```

The title bar reads **MedForge M5**. Close a running MedForge before linking, or the linker cannot replace `medforge.exe`.

1. Choose **File → Import CT folder…** and select `tests/fixtures/synthetic-ct`. The banner reads `Research/demo only — not for clinical use.`
2. In Properties, scroll to the measurement block. The notice says the numbers are not validated and are limited by CT resolution, segmentation, and landmark placement. Leave **Snap to surface** on and Snap at 2.0 mm. Snap applies only after a surface exists, and a point farther than the tolerance stays where you clicked.
3. Choose **Place** and click the axial image twice. Each click adds `L1`, `L2`, and so on. The selected landmark is highlighted. Sagittal and coronal show it when that plane’s slice is within half a voxel. The 3D pane draws the same points.
4. Choose **Distance** and click near two landmarks, within 8 mm of each. The table adds a row in millimeters. On a slice that contains both points, the MPR view draws the segment.
5. Place a third landmark. Choose **Angle** and click the first point, the vertex, then the third point. The angle is at the middle landmark and is reported in degrees.
6. Choose **Area** and click the same three landmarks. With no surface, the value reads `No surface`. Choose **Build surface** at 200 HU so the 400 HU voxel becomes a mesh. The area row updates in mm². It is the mesh–plane intersection, not the triangle of the three points. Collinear landmarks read `Landmarks are collinear`. A plane that misses the mesh reads `Plane misses the surface`.
7. Select a landmark and edit **L**, **P**, or **S**. The distance, angle, and area recompute. **Undo measure** and **Redo measure** restore the definitions. **Delete landmark** removes that point and any measurement that used it. Edit → Undo still applies only to the mask.
8. Choose **File → Save measurements…** and write a `.mfmeasure` file. It records the series and the landmark definitions, not the derived numbers. Load it into the same series and the table fills in again. A file from another series is rejected. **File → Export measurement report…** writes a text report and a sibling `.csv` with units, spacing, and the series UID. Both say the result is research-only and not validated.

## Capture

```powershell
$env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_QPA_FONTDIR = "C:\Windows\Fonts"
$env:QT_SCALE_FACTOR = "1"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M5
$env:QT_SCALE_FACTOR = "1.5"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M5
```

Offscreen images show the software volume box, not the ray cast. Inspect `measure-overlay` at 1280×800, 1920×1080, and 2560×1440, including the 150% scale copies.
