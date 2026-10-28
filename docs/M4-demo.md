# M4 demo — segmentation

The series in `tests/fixtures/synthetic-ct` is generated, not a patient study. Four axial slices, 8×6 pixels. Spacing is 0.8 × 0.5 × 2 mm. Air is −1000 HU. Slice `k` paints column `i = k` at 80 HU. Pixel (3, 2) on slice 1 is 400 HU.

## Run

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
.\build\windows-msvc\bin\Debug\medforge.exe
```

The title bar reads **MedForge M4**. Close a running MedForge before linking, or the linker cannot replace `medforge.exe`.

1. Choose **File → Import CT folder…** and select `tests/fixtures/synthetic-ct`.
2. In Properties, scroll to the segmentation block. The line under the volume note says this is research segmentation, not a clinical contour. Leave Min at 200 HU and Max at 1500 HU. The preview line reads `1 voxel in range`.
3. Choose **Apply threshold**. Studies gains Segmentation 1, and the status reads `Unsaved mask`. Scroll the axial view to slice `k = 1` (or click the light voxel). Axial, sagittal, and coronal show that 400 HU voxel tinted. The 80 HU bars stay gray.
4. Choose **Edit → Undo**. The tint and the segmentation row clear, and the status returns to `Mask saved`. **Edit → Redo** puts the voxel back.
5. Choose Label 2, then Paint, and drag on a 2D slice. That stroke is label 2. Erase removes voxels under the brush. The brush stays on the slice you are viewing. Switch back to Label 1 to mesh the first region; the studies count still includes both labels.
6. Put the crosshair on air and choose **Grow from crosshair**. The banner warns `Seed is outside the HU range`. Move onto the 400 HU voxel, choose 6-connected or 26-connected, and optionally **Limit to axial slice**, then grow again. **Cancel** during a longer grow leaves the previous mask in place.
7. Choose **Mesh from label**. The 3D pane shows the label surface, and the dock records the triangle count plus `Label 1` and the series UID. **File → Save mask…** writes a `.mfmask`. Load it back into the same series; the voxels match. A mask from a different series is rejected.

Turn on **Preview HU range** to tint the current window without writing the mask. Tool choices, the HU limits, the brush radius, connectivity, and the axial limit are restored the next time the workspace opens.

## Capture

```powershell
$env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_QPA_FONTDIR = "C:\Windows\Fonts"
$env:QT_SCALE_FACTOR = "1"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M4
$env:QT_SCALE_FACTOR = "1.5"
.\build\windows-msvc\bin\Debug\medforge.exe --capture docs\ui\evidence\M4
```

Offscreen images show the software volume box and mesh, not the ray cast. Inspect `segment-overlay`, `segment-label2`, `segment-seed`, and `segment-mesh`.
