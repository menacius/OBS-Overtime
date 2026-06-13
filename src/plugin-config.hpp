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

    // Colors are stored as 0xAARRGGBB.
    uint32_t textColor = 0xFFFFFFFF;
    uint32_t backgroundColor = 0xFF000000;
    int backgroundOpacity = 160;  // 0-255 applied on top of bg color alpha
    int backgroundPadding = 8;    // px

    OverlayPosition position = OverlayPosition::BottomRight;
    int edgeOffset = 24;  // px margin from the chosen edge(s)

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
