#pragma once

namespace medforge::imaging {

// DICOM import and image volumes arrive in M2. This target exists so later code
// depends on imaging rather than reaching into UI or rendering.
const char* moduleName();

} // namespace medforge::imaging
