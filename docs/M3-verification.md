# M3 verification

Status: Windows debug gate passed on 2026-10-28. Linux and GitHub Actions were not run for this milestone.

## Scope

Volume ray casting, CT transfer presets, a clip plane, anatomical cameras, and a Flying Edges isosurface in patient LPS millimeters. The surface is shown with triangle count, enclosed volume, memory, and elapsed time. It is not written to a file. STL and STEP export remain M6.

Not in this milestone: segmentation, measurements, clinical claims, compressed DICOM, and enhanced multi-frame CT.

## Tests written first

`anatomicalCamera` failed to compile while the function was declared before `CameraPose`. MSVC treated the name as `int`.

`rendering_isosurface_test` crashed in the progress callback because the VTK client pointer was read from the call-data argument. After that fix, the anisotropic pole normal was below 0.4 until the gradient was a trilinear central difference and the sample was the +Z vertex nearest the axis.

`ui_volume_workspace_test` was added for the finished workspace behavior and passed on its first run: volume pane instead of the sample, Bone preset, anterior and superior cameras, marker mesh in patient space, clip reducing the triangle count, cancel keeping the previous mesh, reload clearing it, and a 3000 HU threshold warning.

The debug executable then failed to start (`0xC0000135`). Surface extraction had linked the release VTK import libraries, which are not deployed next to the app. Debug builds now point those targets at `third_party/dist/vtk-9.4.2-windows-msvc-debug` before linking. The suite was rerun after that link.

## Commands

`cmake --build --preset windows-msvc-debug`

`ctest --preset windows-msvc-debug --output-on-failure` — 23/23 passed.

`medforge.exe --capture docs/ui/evidence/M3` at `QT_SCALE_FACTOR` 1 and 1.5, with `QT_QPA_PLATFORM=offscreen`. Offscreen Qt prints `propagateSizeHints` and still writes the PNGs.

## What was inspected

`docs/ui/evidence/M3/volume-populated-1280x800-scale100.png`, the 1920×1080 and 2560×1440 siblings, and `volume-populated-1280x800-scale150.png`: axial bar, coronal staircase, sagittal pane in millimeter aspect, orientation letters, crosshair, and readout `i=4 j=3 k=2  L 13.20  P -18.50  S 4.00 mm  HU -1000`. Properties show Soft/Lung/Bone, Volume/Surface/Clip, threshold 200 HU, camera letters A–I, and “No surface”. The research lines are visible and are not clinical claims. The 3D pane draws the volume bounds and the status “Volume ray casting needs a display”.

`volume-building-1280x800-scale100.png`: Tasks reads “Building surface”, progress is 0%, and Cancel is enabled.

`volume-no-surface-1280x800-scale100.png`: warning banner “No surface at this threshold” at 3000 HU, and the mesh count stays “No surface”.

`volume-mesh-1280x800-scale100.png`, `volume-mesh-1920x1080-scale100.png`, and `volume-mesh-2560x1440-scale150.png`: threshold 0, Studies shows Surface 118, and Properties reads “118 triangles”, “Volume 0.7 mm³”, “0.00 MB volume, 0.01 MB mesh”, “1 ms”. The mesh is a small mark inside the volume box because the phantom is mostly air. Labels in the properties dock are complete at 1280, 1920, 2560, and at 150%.

GUI capture still skips the VTK window and paints the software preview. `cpuRayCastPaintsTheViewport` renders two separated markers with the fixed-point mapper and requires similar energy on the left and right of the framebuffer. An earlier quad order left the right triangle undrawn, which looks like a planar cut. That run did not print “no override found”. The MedForge window was not inspected on a display.

## Acceptance

| Gate | Status |
| --- | --- |
| Red/green evidence | PASS |
| Windows debug build and 23 tests | PASS |
| Linux build | NOT VERIFIED |
| GitHub Actions | NOT VERIFIED |
| Synthetic sphere radius and volume | PASS (`rendering_isosurface_test`) |
| Anisotropic spacing and outward normals | PASS |
| Clip, presets, cameras, cancel, reload | PASS |
| Screenshots inspected at 1280, 1920, 2560 and at 150% | PASS for chrome and software preview |
| CPU volume ray-cast pixels | PASS (`cpuRayCastPaintsTheViewport`) |
| GPU volume ray-cast pixels | NOT VERIFIED |
| On-display MedForge window | NOT VERIFIED |
| No patient data, no clinical claim, no file export | PASS |

## Known limits

CI does not install VTK. `rendering_isosurface_test` skips extraction with “VTK is required to extract an isosurface”. The workspace reports “VTK is not available” and does not invent a mesh.

On a display, the 3D pane uses `vtkFixedPointVolumeRayCastMapper` and the status line reads “CPU ray casting”. This VTK package does not register `vtkOpenGLGPUVolumeRayCastMapper` or `vtkOpenGLRayCastImageDisplayHelper`, so MedForge registers its own display helper. `volumeMapperOverrideIsRegistered` checks that `vtkRayCastImageDisplayHelper::New()` returns that helper. Close a running MedForge and start the newly linked `medforge.exe` before expecting the warning to disappear.

A threshold with no crossings warns and clears the mesh. Cancel keeps the mesh already on screen. Importing another series cancels an in-flight extraction and clears the mesh. The default threshold is 200 HU. The synthetic phantom’s 80 HU bars need 0 HU; its 400 HU marker is the surface at 200 HU.

The clip plane is the axial plane through the crosshair. It keeps the half-space on the negative side of the slice direction and is applied to both the mesh and the volume mapper.
