# Screenshot capture and inspection

Windows is the primary target. Capture uses the offscreen Qt platform so a 2560×1440 widget does not require a matching monitor. DPI checks use `QT_SCALE_FACTOR` (`1` and `1.5`) before the process starts. The manifest next to the PNGs records logical size, image pixel size, and device pixel ratio.

## Capture

From a configured build:

```powershell
powershell -ExecutionPolicy Bypass -File tools/capture_screenshots.ps1 -BuildDir build/windows-mingw-debug
```

Or directly:

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_QPA_FONTDIR = "C:\Windows\Fonts"
$env:QT_SCALE_FACTOR = "1"
.\build\windows-mingw-debug\bin\medforge.exe --capture docs\ui\evidence\M0
$env:QT_SCALE_FACTOR = "1.5"
.\build\windows-mingw-debug\bin\medforge.exe --capture docs\ui\evidence\M0
```

On Linux, from a `linux-gcc-debug` build:

```bash
bash tools/capture_screenshots.sh build/linux-gcc-debug
```

The command writes gallery, M1 reference, foundation, and confirmation-dialog PNGs plus `manifest-scale100.txt` and `manifest-scale150.txt`. Linux uses DejaVu when Segoe UI and Consolas are not installed. Do not set `QT_QPA_FONTDIR` to a Windows font directory on Linux.

## Inspection

Open every PNG listed in the manifest. Check:

- research notice is readable
- text is not clipped inside controls
- disabled, selected, invalid, and empty states are distinguishable without relying on color alone
- toolbar and dialog controls meet a usable hit size
- 1280×800, 1920×1080, and 2560×1440 layouts keep their regions
- 150% scale does not crop the confirmation dialog or icon strokes

Approved M1 layout baselines are copied to `docs/ui/reference/`. Observed captures stay in `docs/ui/evidence/M0/`. Record defects in `docs/M0-verification.md`. A capture without this inspection is not a pass.

Gallery content below the first screen scrolls. Confirm the scrollbar is present and inspect `gallery-2560x1440-scale100.png` for the sections that sit lower at 1280×800.

## Automated check

`ui_screenshot_capture_test` writes the 100% set into a temporary directory and checks that the files exist and are non-trivial. It does not judge visual quality.
