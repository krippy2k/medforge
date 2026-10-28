# M2 demo — synthetic CT

The series in `tests/fixtures/synthetic-ct` is generated, not a patient study. Four axial slices, 8×6 pixels. Spacing is 0.8 × 0.5 × 2 mm. Air is −1000 HU. Slice `k` paints column `i = k` at 80 HU. Pixel (3, 2) on slice 1 is 400 HU.

## Run

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
.\build\windows-msvc\bin\Debug\medforge.exe
```

The title bar reads **MedForge M2**.

1. Choose **File → Import CT folder…** and select `tests/fixtures/synthetic-ct`.
2. Axial, sagittal, and coronal show the series. The status bar reads voxel index, LPS millimeters, and HU.
3. Click a view to move the linked crosshair. The mouse wheel, Up, or Down scrolls that plane. Left and Right move the crosshair across the slice.
4. Use Soft, Lung, Bone, or Brain, or type Window and Level. These are display presets, not a clinical window.
5. During a longer import, **Cancel** in the Tasks dock stops the read. The window stays responsive.

A folder with two slices at the same position shows **Duplicate slice positions** and leaves the views empty. JPEG and the other named compressed syntaxes are recognized and not decoded.
