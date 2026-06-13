#include "time-utils.hpp"

#include <cstdio>
#include <cstdlib>

std::string formatDuration(int64_t milliseconds)
{
    if (milliseconds < 0)
        milliseconds = 0;

    int64_t totalSeconds = milliseconds / 1000;
    int64_t hours = totalSeconds / 3600;
    int64_t minutes = (totalSeconds % 3600) / 60;
    int64_t seconds = totalSeconds % 60;

    char buf[32];
    if (hours > 0) {
        snprintf(buf, sizeof(buf), "%lld:%02lld:%02lld",
                 (long long)hours, (long long)minutes, (long long)seconds);
    } else {
        snprintf(buf, sizeof(buf), "%lld:%02lld",
                 (long long)minutes, (long long)seconds);
    }
    return std::string(buf);
}
