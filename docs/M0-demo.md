# M0 demo procedure

Research/demo only. Not for clinical use. No patient data is loaded.

1. Configure and build the debug preset (see the README).
2. Run `ctest --preset windows-msvc-debug --output-on-failure` and show the coordinate, command-history, and UI tests.
3. Start `medforge.exe`. Point out the research notice, then open the component gallery from the button or Developer menu.
4. In the gallery, change the segmented control, enter an out-of-range spacing value, select a browser row, dismiss a notification, and run the sample task. Cancel it before it finishes and show that the status line reports cancellation.
5. Open **Delete landmark…** and cancel the dialog. The default action is Cancel.
6. Return to the foundation window and open the M1 layout reference. Walk the studies column, 2×2 viewports, properties column, and LPS status row.
7. Show `docs/ui/evidence/M0/` and the manifests for 1280×800, 1920×1080, and 2560×1440 at 100% and 150% scale.

Equivalent launches:

```text
medforge.exe --gallery
medforge.exe --reference
medforge.exe --capture docs/ui/evidence/M0
```
