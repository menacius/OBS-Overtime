#pragma once

#include <string>
#include <cstdint>

// Formats a duration given in milliseconds as H:MM:SS or M:SS.
std::string formatDuration(int64_t milliseconds);
