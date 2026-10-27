#!/usr/bin/env bash
set -euo pipefail
cd /mnt/d/projects/medforge
exec bash tools/build_vtk.sh \
    /mnt/d/projects/medforge/third_party/src/VTK-9.4.2 \
    "${HOME}/vtk/build-9.4.2" \
    "${HOME}/vtk/9.4.2"
