# M2 verification

Status: Windows debug gate passed on 2026-10-27. Linux and GitHub Actions were not re-run for this slice.

## Scope

Folder import of uncompressed CT, series selection, Hounsfield display, and linked axial/sagittal/coronal views. The documented series is `tests/fixtures/synthetic-ct`.

Not in this milestone: volume rendering (M3), decoding compressed transfer syntaxes, and a clinical window claim. The in-tree reader covers explicit and implicit VR little endian. DCMTK is still only the planned pin in `dependencies/versions.lock.json`.

## Tests written first

`imaging_ct_series_test` failed to compile before `CtSeries.h` existed.

`readsImplicitLittleEndianCt` failed with status Truncated (2) before file-meta elements after the transfer syntax were parsed as explicit VR.

`ui_workspace_window_test` failed while the title was `MedForge` instead of `MedForge M2`.

Later M2 tests were added with the implementation and then run red only where a behavior was wrong: the synthetic phantom’s column 0 on slice 0 is 80 HU, not air, so the first import assertion failed and was pointed at an air pixel.

`readsASequenceAndPaddedDecimalStrings` failed with MissingAttribute (4), and `readsADatasetWithoutAFilePreamble` failed with NotDicom (1), before the reader skipped undefined-length sequences, trimmed decimal tokens, and accepted a dataset that has no part-10 preamble. A part-10 file that still cannot be parsed is reported as found, rather than as an empty folder.

`reformattedViewsUseMillimeterSpacing` failed while sagittal and coronal pictures kept the voxel-count aspect. The phantom is 0.8 × 0.5 × 2 mm, so those views are drawn with slice spacing included.

## Commands

`ctest --preset windows-msvc-debug` — 20/20 passed.

`medforge.exe --capture docs/ui/evidence/M2` at `QT_SCALE_FACTOR` 1 and 1.5. Offscreen Qt prints `propagateSizeHints` and still writes the PNGs.

## What was inspected

`docs/ui/evidence/M2/mpr-empty-1280x800-scale100.png`: three empty planes, research notice, Import CT folder, Properties title fully visible.

`mpr-importing-1280x800-scale100.png`: status “Importing CT series”, progress at 0%, Cancel enabled, views still empty.

`mpr-invalid-1280x800-scale100.png`: error banner “Duplicate slice positions”, views stay empty.

`mpr-populated-1280x800-scale100.png`, `mpr-populated-1920x1080-scale100.png`, `mpr-populated-2560x1440-scale100.png`, and `mpr-populated-1280x800-scale150.png`: axial bar, coronal staircase, orientation letters, cyan crosshair, window 400 / level 40, and readout `i=4 j=3 k=2  L 13.20  P -18.50  S 4.00 mm  HU -1000`. That HU is air at the default center. The 80 HU bars are the bright column and the coronal steps. Preset buttons and the series UID fit. 150% uses a 1920×1200 image for the 1280×800 logical window and does not clip the toolbar or the readout.

The center sagittal plane of this phantom is air, so that pane is black aside from the crosshair. Scrolling onto column 0–3 shows the bar.

## Acceptance

| Gate | Status |
| --- | --- |
| Red/green evidence | PASS |
| Windows debug build and 20 tests | PASS |
| Linux build | NOT VERIFIED |
| GitHub Actions | NOT VERIFIED |
| Empty, importing, invalid, and populated states | PASS |
| Linked crosshair, scroll, keyboard, window/level | PASS |
| Screenshots inspected at 1280, 1920, 2560 and at 150% | PASS |
| No patient data, no clinical claim | PASS |
| Import stays off the GUI thread and can be cancelled | PASS |

## Known limits

Compressed syntaxes (JPEG baseline, JPEG lossless, JPEG-LS, JPEG 2000, JPEG 2000 lossless, RLE) are named and rejected. Big-endian uncompressed pixels are rejected. A folder with several series loads the largest one. Planes are orthogonal in voxel index space and labeled from the direction cosines, which matches axial/sagittal/coronal for a standard axial acquisition.

Classic single-frame CT is read when the file is explicit or implicit VR little endian, including files whose sequences have undefined length and files that omit the 128-byte preamble. Enhanced multi-frame CT, whose geometry lives in functional-group sequences, is still rejected with a message that a DICOM file was found.
