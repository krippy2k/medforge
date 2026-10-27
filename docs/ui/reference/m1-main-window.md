# M1 main window reference

These images are the M0 layout baseline for M1. They are annotations, not a working workspace.

- `m1-main-window-1280x800.png`
- `m1-main-window-1920x1080.png`
- `m1-main-window-2560x1440.png`

## Regions

- Top: research notice, menu names, toolbar-slot caption.
- Left: Studies dock with a hierarchical browser.
- Center: 2×2 viewports labeled Axial, Sagittal, Coronal, and 3D.
- Right: Properties dock.
- Bottom: status, LPS millimeters, and an idle progress/cancel row.

M1 should reuse the M0 tokens and components and replace these annotations with real docks and a VTK viewport. The reference does not implement docking, camera control, or DICOM import.

## Baseline decision

Keep the `DESIGN_SYSTEM.md` palette, 4 px spacing grid, Segoe UI at 10 pt, and Consolas for numeric fields. Qt Fusion plus the generated token stylesheet is the shared style. Primary buttons use dark text on the accent color because white text on `#4BA3C7` does not meet readable contrast.
