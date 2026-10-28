#pragma once

namespace medforge::core {

inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 1;
inline constexpr int kVersionPatch = 0;
inline constexpr const char* kVersionString = "0.1.0";
inline constexpr const char* kMilestone = "M3";
inline constexpr const char* kResearchNotice = "Research/demo only — not for clinical use.";

const char* moduleName();

} // namespace medforge::core
