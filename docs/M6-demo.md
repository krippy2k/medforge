# M6 demo — mesh repair and STL export

The series in `tests/fixtures/synthetic-ct` is generated, not a patient study. Four axial slices, 8×6 pixels. Spacing is 0.8 × 0.5 × 2 mm. Air is −1000 HU. Slice `k` paints column `i = k` at 80 HU. Pixel (3, 2) on slice 1 is 400 HU.

## Run

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
.\build\windows-msvc\bin\Debug\medforge.exe
```

The title bar reads **MedForge M6**. Close a running MedForge before linking, or the linker cannot replace `medforge.exe`.

1. Choose **File → Import CT folder…** and select `tests/fixtures/synthetic-ct`.
2. In Properties, set Threshold to 0 HU and choose **Build surface**. Studies gains a Surface row. The 3D pane shows the mesh. The dock reports whether it is watertight, the area in mm², the volume in mm³ when every edge belongs to two triangles, the component count, and the open and non-manifold edge counts. Volume stays unavailable while the mesh is open. The line `STL units: millimeters` is the unit label. STL files do not carry a unit field of their own.
3. Leave Smooth selected, Iterations at 1, and choose **Preview repair**. The 3D pane shows the candidate. The diagnostics keep the source numbers and say `Preview — not applied`. **Cancel preview** restores the source view. **Apply repair** commits the candidate. **Undo mesh** and **Redo mesh** step through committed repairs. Edit → Undo still applies only to the mask. Building a new surface clears the mesh undo stack.
4. Choose Decimate and set Keep to 50%. Preview again. The triangle count drops. Cancel if you want the source back.
5. Choose Fill holes and set Hole to 12 edges. Preview fills only closed holes at or under that limit. A larger hole, or a boundary that is not a closed loop, is refused and the source mesh is left unchanged. The banner states the reason.
6. Choose **File → Export STL…** and write a `.stl` file. The banner names the triangle count, says the units are millimeters, and repeats the series UID. Export is refused while a preview is showing. The file is binary STL. Its header begins `MedForge binary STL, units millimeters`. The workstation reads the file back and rejects the export if the bounds do not match.

## Capture

```powershell
$env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_QPA_FONTDIR = "C:\Windows\Fonts"
$env:QT_SCALE_FACTOR = "1"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M6
$env:QT_SCALE_FACTOR = "1.5"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M6
```

Offscreen images show the software volume box and mesh, not the ray cast. Inspect `mesh-diagnostics` at 1280×800, 1920×1080, and 2560×1440, plus `mesh-preview` and `mesh-export`.
