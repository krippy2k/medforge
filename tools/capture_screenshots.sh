#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build/linux-gcc-debug}"
output_dir="${2:-docs/ui/evidence/M0}"
exe="${build_dir}/bin/medforge"

if [[ ! -x "${exe}" ]]; then
    echo "Executable not found: ${exe}" >&2
    exit 1
fi

export QT_QPA_PLATFORM=offscreen
export QT_ASSUME_STDERR_HAS_CONSOLE=1
mkdir -p "${output_dir}"

for scale in 1 1.5; do
    QT_SCALE_FACTOR="${scale}" "${exe}" --capture "${output_dir}"
done

echo "Screenshots written to ${output_dir}"
echo "Inspect the PNGs and manifests before treating visual QA as passed."
