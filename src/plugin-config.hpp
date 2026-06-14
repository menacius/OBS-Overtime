#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Overlay anchor positions relative to the target projector window.
enum class OverlayPosition {
    Top,
    Bottom,
    Left,
    Right,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center,
};

struct OverlayValuePlacement {
    OverlayPosition position = OverlayPosition::BottomRight;
    int edgeOffset = 24;
};

// Persistent configuration for the Projector Time Overlay plugin.
// All settings are saved to / loaded from a JSON file in the module
// config directory so they survive across OBS sessions.
struct PluginConfig {
    bool enabled = true;

    // Font
    std::string fontFamily = "Sans Serif";
    int fontSize = 28;

    // Which values to display (multiple may be enabled simultaneously)
    bool showStreaming = true;
    bool showRecording = false;
    bool showMediaElapsed = false;
    bool showMediaRemaining = false;

    // When enabled, media elapsed/remaining values are shown only if the
    // selected media source is both active/visible and currently playing.
    bool mediaTimesOnlyWhenActivePlaying = false;

    // Media end warning. When remaining time is at or below this threshold,
    // media time backgrounds blink once per second between the normal
    // background and this warning background. Set threshold to 0 to disable.
    int mediaWarningThresholdSeconds = 0;
    uint32_t mediaWarningBackgroundColor = 0xFFFF0000;
    int mediaWarningBackgroundOpacity = 200;

    // Periodic stream/record time warning. At each interval, the selected
    // timer background changes for the configured duration.
    bool timeWarningStreaming = false;
    bool timeWarningRecording = false;
    int timeWarningIntervalSeconds = 900;
    int timeWarningDurationSeconds = 5;
    uint32_t timeWarningBackgroundColor = 0xFFFFAA00;
    int timeWarningBackgroundOpacity = 220;

    // Colors are stored as 0xAARRGGBB.
    uint32_t textColor = 0xFFFFFFFF;
    uint32_t backgroundColor = 0xFF000000;
    int backgroundOpacity = 160;  // 0-255 applied on top of bg color alpha
    int backgroundPadding = 8;    // px

    // Per-value placement. Each displayed value can be positioned independently.
    // The legacy global position/edge_offset keys are ignored on save.
    OverlayValuePlacement streamingPlacement{OverlayPosition::BottomRight, 24};
    OverlayValuePlacement recordingPlacement{OverlayPosition::BottomLeft, 24};
    OverlayValuePlacement mediaElapsedPlacement{OverlayPosition::TopRight, 24};
    OverlayValuePlacement mediaRemainingPlacement{OverlayPosition::TopLeft, 24};

    // Names of projector windows the overlay should attach to.
    // Empty means "all detected projectors".
    std::vector<std::string> targetProjectors;

    // Names of media sources enabled in the dock for elapsed/remaining time.
    std::vector<std::string> enabledMediaSources;

    // Returns the shared singleton config instance.
    static PluginConfig &instance();

    // Load from disk. Safe to call when no file exists (keeps defaults).
    void load();

    // Persist current state to disk.
    void save() const;

    bool isMediaSourceEnabled(const std::string &name) const;
    void setMediaSourceEnabled(const std::string &name, bool enabled);

    bool isProjectorTargeted(const std::string &name) const;
};

const char *overlayPositionToString(OverlayPosition pos);
OverlayPosition overlayPositionFromString(const char *str);
