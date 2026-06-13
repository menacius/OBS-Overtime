#pragma once

#include <obs.h>

#include <string>
#include <vector>
#include <cstdint>

// Describes a media-capable OBS source for the dock list.
struct MediaSourceInfo {
    std::string name;
    bool active = false;   // currently visible/active on program
    bool playing = false;  // media state is playing
};

// MediaManager enumerates media-capable sources and resolves which one
// should be used for elapsed/remaining time according to the configured
// priority rules. All OBS source access is reference-counted and guarded
// so deletion of a selected source never causes a crash.
class MediaManager {
public:
    // Returns every media-capable source currently present in OBS.
    static std::vector<MediaSourceInfo> enumerateMediaSources();

    // Returns true if the source type supports the media controls API.
    static bool isMediaSource(obs_source_t *source);

    // Resolves the active media source name based on the enabled list and
    // priority rules. Returns empty string if none is available.
    static std::string resolveActiveSourceName();

    // Fills elapsed/remaining (ms) for the resolved source.
    // Returns false if no usable source exists.
    static bool getTimes(std::string &outName, int64_t &elapsedMs,
                         int64_t &remainingMs);
};
