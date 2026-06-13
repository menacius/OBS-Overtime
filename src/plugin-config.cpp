#include "plugin-config.hpp"

#include <obs-module.h>
#include <util/config-file.h>
#include <util/platform.h>
#include <obs-data.h>

#include <algorithm>
#include <cstring>
#include <string>

static const char *kConfigFile = "config.json";

const char *overlayPositionToString(OverlayPosition pos)
{
    switch (pos) {
    case OverlayPosition::Top: return "top";
    case OverlayPosition::Bottom: return "bottom";
    case OverlayPosition::Left: return "left";
    case OverlayPosition::Right: return "right";
    case OverlayPosition::TopLeft: return "top-left";
    case OverlayPosition::TopRight: return "top-right";
    case OverlayPosition::BottomLeft: return "bottom-left";
    case OverlayPosition::BottomRight: return "bottom-right";
    case OverlayPosition::Center: return "center";
    }
    return "bottom-right";
}

OverlayPosition overlayPositionFromString(const char *str)
{
    if (!str) return OverlayPosition::BottomRight;
    if (strcmp(str, "top") == 0) return OverlayPosition::Top;
    if (strcmp(str, "bottom") == 0) return OverlayPosition::Bottom;
    if (strcmp(str, "left") == 0) return OverlayPosition::Left;
    if (strcmp(str, "right") == 0) return OverlayPosition::Right;
    if (strcmp(str, "top-left") == 0) return OverlayPosition::TopLeft;
    if (strcmp(str, "top-right") == 0) return OverlayPosition::TopRight;
    if (strcmp(str, "bottom-left") == 0) return OverlayPosition::BottomLeft;
    if (strcmp(str, "bottom-right") == 0) return OverlayPosition::BottomRight;
    if (strcmp(str, "center") == 0) return OverlayPosition::Center;
    return OverlayPosition::BottomRight;
}

PluginConfig &PluginConfig::instance()
{
    static PluginConfig cfg;
    return cfg;
}

bool PluginConfig::isMediaSourceEnabled(const std::string &name) const
{
    return std::find(enabledMediaSources.begin(), enabledMediaSources.end(),
                     name) != enabledMediaSources.end();
}

void PluginConfig::setMediaSourceEnabled(const std::string &name, bool enabled)
{
    auto it = std::find(enabledMediaSources.begin(),
                        enabledMediaSources.end(), name);
    if (enabled && it == enabledMediaSources.end()) {
        enabledMediaSources.push_back(name);
    } else if (!enabled && it != enabledMediaSources.end()) {
        enabledMediaSources.erase(it);
    }
}

bool PluginConfig::isProjectorTargeted(const std::string &name) const
{
    if (targetProjectors.empty())
        return true;  // empty == all projectors
    return std::find(targetProjectors.begin(), targetProjectors.end(),
                     name) != targetProjectors.end();
}


static void load_placement(obs_data_t *data, const char *positionKey,
                           const char *offsetKey,
                           OverlayValuePlacement &placement)
{
    if (obs_data_has_user_value(data, positionKey)) {
        placement.position = overlayPositionFromString(
            obs_data_get_string(data, positionKey));
    }
    if (obs_data_has_user_value(data, offsetKey))
        placement.edgeOffset = (int)obs_data_get_int(data, offsetKey);
}

static void save_placement(obs_data_t *data, const char *positionKey,
                           const char *offsetKey,
                           const OverlayValuePlacement &placement)
{
    obs_data_set_string(data, positionKey,
                        overlayPositionToString(placement.position));
    obs_data_set_int(data, offsetKey, placement.edgeOffset);
}

static std::string config_path()
{
    char *path = obs_module_config_path(kConfigFile);
    std::string result = path ? path : "";
    bfree(path);
    return result;
}

void PluginConfig::load()
{
    std::string path = config_path();
    if (path.empty())
        return;

    obs_data_t *data = obs_data_create_from_json_file(path.c_str());
    if (!data)
        return;  // No file yet: keep defaults.

    obs_data_set_default_bool(data, "enabled", enabled);
    enabled = obs_data_get_bool(data, "enabled");

    const char *family = obs_data_get_string(data, "font_family");
    if (family && *family)
        fontFamily = family;
    if (obs_data_has_user_value(data, "font_size"))
        fontSize = (int)obs_data_get_int(data, "font_size");

    showStreaming = obs_data_get_bool(data, "show_streaming");
    showRecording = obs_data_get_bool(data, "show_recording");
    showMediaElapsed = obs_data_get_bool(data, "show_media_elapsed");
    showMediaRemaining = obs_data_get_bool(data, "show_media_remaining");
    if (obs_data_has_user_value(data, "media_times_only_when_active_playing"))
        mediaTimesOnlyWhenActivePlaying =
            obs_data_get_bool(data, "media_times_only_when_active_playing");
    if (obs_data_has_user_value(data, "media_warning_threshold_seconds"))
        mediaWarningThresholdSeconds =
            (int)obs_data_get_int(data, "media_warning_threshold_seconds");
    if (obs_data_has_user_value(data, "media_warning_background_color"))
        mediaWarningBackgroundColor =
            (uint32_t)obs_data_get_int(data, "media_warning_background_color");
    if (obs_data_has_user_value(data, "media_warning_background_opacity"))
        mediaWarningBackgroundOpacity =
            (int)obs_data_get_int(data, "media_warning_background_opacity");

    if (obs_data_has_user_value(data, "text_color"))
        textColor = (uint32_t)obs_data_get_int(data, "text_color");
    if (obs_data_has_user_value(data, "background_color"))
        backgroundColor = (uint32_t)obs_data_get_int(data, "background_color");
    if (obs_data_has_user_value(data, "background_opacity"))
        backgroundOpacity = (int)obs_data_get_int(data, "background_opacity");
    if (obs_data_has_user_value(data, "background_padding"))
        backgroundPadding = (int)obs_data_get_int(data, "background_padding");

    load_placement(data, "streaming_position", "streaming_edge_offset", streamingPlacement);
    load_placement(data, "recording_position", "recording_edge_offset", recordingPlacement);
    load_placement(data, "media_elapsed_position", "media_elapsed_edge_offset", mediaElapsedPlacement);
    load_placement(data, "media_remaining_position", "media_remaining_edge_offset", mediaRemainingPlacement);

    targetProjectors.clear();
    obs_data_array_t *projArr = obs_data_get_array(data, "target_projectors");
    if (projArr) {
        size_t count = obs_data_array_count(projArr);
        for (size_t i = 0; i < count; i++) {
            obs_data_t *item = obs_data_array_item(projArr, i);
            const char *n = obs_data_get_string(item, "name");
            if (n && *n)
                targetProjectors.emplace_back(n);
            obs_data_release(item);
        }
        obs_data_array_release(projArr);
    }

    enabledMediaSources.clear();
    obs_data_array_t *mediaArr = obs_data_get_array(data, "media_sources");
    if (mediaArr) {
        size_t count = obs_data_array_count(mediaArr);
        for (size_t i = 0; i < count; i++) {
            obs_data_t *item = obs_data_array_item(mediaArr, i);
            const char *n = obs_data_get_string(item, "name");
            if (n && *n)
                enabledMediaSources.emplace_back(n);
            obs_data_release(item);
        }
        obs_data_array_release(mediaArr);
    }

    obs_data_release(data);
}

void PluginConfig::save() const
{
    std::string path = config_path();
    if (path.empty())
        return;

    // Ensure the module config directory exists.
    char *dir = obs_module_config_path("");
    if (dir) {
        os_mkdirs(dir);
        bfree(dir);
    }

    obs_data_t *data = obs_data_create();
    obs_data_set_bool(data, "enabled", enabled);
    obs_data_set_string(data, "font_family", fontFamily.c_str());
    obs_data_set_int(data, "font_size", fontSize);
    obs_data_set_bool(data, "show_streaming", showStreaming);
    obs_data_set_bool(data, "show_recording", showRecording);
    obs_data_set_bool(data, "show_media_elapsed", showMediaElapsed);
    obs_data_set_bool(data, "show_media_remaining", showMediaRemaining);
    obs_data_set_bool(data, "media_times_only_when_active_playing",
                      mediaTimesOnlyWhenActivePlaying);
    obs_data_set_int(data, "media_warning_threshold_seconds",
                     mediaWarningThresholdSeconds);
    obs_data_set_int(data, "media_warning_background_color",
                     mediaWarningBackgroundColor);
    obs_data_set_int(data, "media_warning_background_opacity",
                     mediaWarningBackgroundOpacity);
    obs_data_set_int(data, "text_color", textColor);
    obs_data_set_int(data, "background_color", backgroundColor);
    obs_data_set_int(data, "background_opacity", backgroundOpacity);
    obs_data_set_int(data, "background_padding", backgroundPadding);
    save_placement(data, "streaming_position", "streaming_edge_offset", streamingPlacement);
    save_placement(data, "recording_position", "recording_edge_offset", recordingPlacement);
    save_placement(data, "media_elapsed_position", "media_elapsed_edge_offset", mediaElapsedPlacement);
    save_placement(data, "media_remaining_position", "media_remaining_edge_offset", mediaRemainingPlacement);

    obs_data_array_t *projArr = obs_data_array_create();
    for (const auto &n : targetProjectors) {
        obs_data_t *item = obs_data_create();
        obs_data_set_string(item, "name", n.c_str());
        obs_data_array_push_back(projArr, item);
        obs_data_release(item);
    }
    obs_data_set_array(data, "target_projectors", projArr);
    obs_data_array_release(projArr);

    obs_data_array_t *mediaArr = obs_data_array_create();
    for (const auto &n : enabledMediaSources) {
        obs_data_t *item = obs_data_create();
        obs_data_set_string(item, "name", n.c_str());
        obs_data_array_push_back(mediaArr, item);
        obs_data_release(item);
    }
    obs_data_set_array(data, "media_sources", mediaArr);
    obs_data_array_release(mediaArr);

    obs_data_save_json_safe(data, path.c_str(), "tmp", "bak");
    obs_data_release(data);
}
