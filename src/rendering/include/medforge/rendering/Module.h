#pragma once

namespace medforge::rendering {

// VTK pipelines arrive in M1. Rendering must not be linked into core.
const char* moduleName();

} // namespace medforge::rendering
