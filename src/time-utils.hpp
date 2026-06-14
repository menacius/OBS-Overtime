#pragma once

#include <string>
#include <cstdint>

// Formats a duration given in milliseconds as HH:MM:SS.
std::string formatDuration(int64_t milliseconds);
