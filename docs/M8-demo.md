# M8 demo — save a project and rehearse the pipeline

Research/demo only. Not for clinical use. The series is the synthetic axial phantom, not a patient study.

## Run

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
$env:PATH = "third_party\dist\occt-7.9.3-windows-msvc-debug\win64\vc14\bind;C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
.\build\windows-msvc\bin\Debug\medforge.exe
```

The title bar reads **MedForge M8**. Close a running MedForge before linking, or the linker cannot replace `medforge.exe`.

`ui_project_workspace_test` writes the phantom, segments it, measures 3.2 mm, exports STL, saves the project, and opens it again. The steps below are the same path by hand. Import a folder written by that phantom (series UID `1.2.826.0.1.3680043.10.543.100`).

1. Choose **File → Import CT folder…**. The axial, sagittal, and coronal views share one crosshair. The status bar shows the LPS position and HU.
2. In Properties, set the segmentation range and choose the threshold action. Label 1 covers the 400 HU voxel. The MPR overlay shows it.
3. Set Threshold to 0 HU and choose **Build surface**. The dock reports the triangle count. Choose **File → Export STL…**. The file is binary STL in millimeters.
4. Place two landmarks and define a distance. The value is in millimeters.
5. Choose **File → Save project…** (`Ctrl+S`). MedForge writes `name.mfproject`, `name.mfmask`, and `name.mfmeasure`. The banner says `Project saved. Not for clinical use.` The project stores a checksum of the CT samples. The image bytes stay in the CT folder.
6. Start MedForge again and choose **File → Open project…** (`Ctrl+Shift+O`). The series, window, threshold, mask, and measurements come back. If the CT folder is missing, the banner says the series folder is missing and nothing is imported. If the voxels changed, the banner says the checksum does not match and the study is cleared.
7. Optional: **File → Export prototype…** still writes STEP for a valid B-rep and STL for a triangle mesh. Both keep the research notice.

An unsigned folder, with `UNSIGNED.txt` and `RELEASE.txt`, is written by:

```powershell
powershell -File tools\package_unsigned.ps1
```

The result is `build/medforge-0.1.0-windows-unsigned`. It is not signed.

## Capture

```powershell
powershell -File tools\capture_screenshots.ps1 -OutputDir docs\ui\evidence\M8
```

Offscreen images show the software volume box, not the ray cast. Inspect `project-saved` at 1280×800, 1920×1080, and 2560×1440, at 100% and at 150%.
