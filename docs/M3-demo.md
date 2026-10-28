# M3 demo — volume and surface

The series in `tests/fixtures/synthetic-ct` is generated, not a patient study. Four axial slices, 8×6 pixels. Spacing is 0.8 × 0.5 × 2 mm. Air is −1000 HU. Slice `k` paints column `i = k` at 80 HU. Pixel (3, 2) on slice 1 is 400 HU.

## Run

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
.\build\windows-msvc\bin\Debug\medforge.exe
```

The title bar reads **MedForge M3**. Close a running MedForge before linking, or the linker cannot replace `medforge.exe`.

1. Choose **File → Import CT folder…** and select `tests/fixtures/synthetic-ct`.
2. Axial, sagittal, and coronal stay linked. The fourth pane replaces the sample cube with the CT bounds. On a display the status line reads “CPU ray casting”. Offscreen capture says a display is required.
3. In Properties, Soft, Lung, and Bone change the volume colors. Volume and Surface show or hide each layer. A, P, L, R, S, and I move the camera in patient LPS. Reset camera returns to the anterior view.
4. Leave Threshold at 200 HU and choose **Build surface**. That isolates the 400 HU marker. Set Threshold to 0 HU and build again to include the 80 HU bars. The dock lists triangle count, enclosed volume, memory, and elapsed time. The Studies list gains a Surface row.
5. Turn on Clip. The plane passes through the crosshair and drops triangles on the positive side of the slice direction. Scroll a 2D view and the plane follows the crosshair.
6. During a longer build, **Cancel** in Tasks stops extraction and leaves the previous surface in place. Importing another folder clears it.

A threshold above every voxel warns **No surface at this threshold**. Nothing is written to STL or STEP.

## Capture

```powershell
$env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_QPA_FONTDIR = "C:\Windows\Fonts"
$env:QT_SCALE_FACTOR = "1"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M3
$env:QT_SCALE_FACTOR = "1.5"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M3
```

Offscreen images show the software volume box and mesh, not the ray cast. Inspect `volume-populated`, `volume-building`, `volume-no-surface`, and `volume-mesh` at 1280×800, 1920×1080, and 2560×1440.
