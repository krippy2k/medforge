# Third-party license inventory

MedForge 0.1.0 does not vendor third-party source. The desktop build links these components dynamically or as toolchain binaries.

| Component | Version used in M0 | License | Notes |
| --- | --- | --- | --- |
| Qt | 6.12.0 | LGPL-3.0, with the Qt commercial license also offered by The Qt Company | Widgets and Test are dynamically linked. The official installer license texts are under the Qt installation `Licenses` directory. Relinking against a compatible Qt is possible because the application does not statically link Qt. |
| Microsoft Visual C++ runtime | Universal CRT, toolset 14.51 | Microsoft Visual Studio license | The default Windows build uses this runtime. |
| CMake | 3.27 or newer (4.4.2 used locally) | BSD-3-Clause | Build tool only. |
| Ninja | 1.13.2 | Apache-2.0 | Build tool only. |
| clang-format / clang-tidy | LLVM 20.1.8 | Apache-2.0 with LLVM exceptions | Development tools only. Not linked into the application. |

## Planned, not linked in M0

These are recorded in `dependencies/versions.lock.json` and are not fetched or compiled for this milestone.

| Component | Planned milestone | Typical license |
| --- | --- | --- |
| VTK 9.4 | M1 rendering | BSD-3-Clause |
| DCMTK 3.6.9 | M2 DICOM import, if selected | BSD-3-Clause |
| ITK 5.4 | M4 segmentation | Apache-2.0 |
| Open CASCADE 7.9.3 | M7 CAD kernel, behind the `cad` adapter. Local debug kit: `third_party/dist/occt-7.9.3-windows-msvc-debug` | LGPL-2.1 |
| CGAL | M6 only if required | GPL-3.0 or commercial; do not add casually |

No patient data, paid assets, or clinical datasets are included.
