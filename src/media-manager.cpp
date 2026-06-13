#include "media-manager.hpp"
#include "plugin-config.hpp"

#include <obs.h>

#include <string>
#include <vector>

bool MediaManager::isMediaSource(obs_source_t *source)
{
    if (!source)
        return false;
    uint32_t flags = obs_source_get_output_flags(source);
    return (flags & OBS_SOURCE_CONTROLLABLE_MEDIA) != 0;
}

namespace {

struct EnumCtx {
    std::vector<MediaSourceInfo> *list;
};

bool enum_media_cb(void *param, obs_source_t *source)
{
    auto *ctx = static_cast<EnumCtx *>(param);
    if (!MediaManager::isMediaSource(source))
        return true;

    MediaSourceInfo info;
    const char *name = obs_source_get_name(source);
    info.name = name ? name : "";
    info.active = obs_source_active(source);
    info.playing = obs_source_media_get_state(source) ==
                   OBS_MEDIA_STATE_PLAYING;
    if (!info.name.empty())
        ctx->list->push_back(info);
    return true;
}

}  // namespace

std::vector<MediaSourceInfo> MediaManager::enumerateMediaSources()
{
    std::vector<MediaSourceInfo> list;
    EnumCtx ctx{&list};
    obs_enum_sources(enum_media_cb, &ctx);
    return list;
}

std::string MediaManager::resolveActiveSourceName()
{
    const PluginConfig &cfg = PluginConfig::instance();
    auto sources = enumerateMediaSources();
    if (sources.empty())
        return "";

    // Build the ordered list of enabled sources following the config order.
    std::vector<const MediaSourceInfo *> enabled;
    for (const auto &name : cfg.enabledMediaSources) {
        for (const auto &src : sources) {
            if (src.name == name) {
                enabled.push_back(&src);
                break;
            }
        }
    }
    if (enabled.empty())
        return "";

    // Priority 1: a currently active/visible AND playing source.
    for (const auto *src : enabled) {
        if (src->active && src->playing)
            return src->name;
    }
    // Priority 2: a currently active/visible source.
    for (const auto *src : enabled) {
        if (src->active)
            return src->name;
    }
    // Priority 3: first enabled source in the list.
    return enabled.front()->name;
}

bool MediaManager::getTimes(std::string &outName, int64_t &elapsedMs,
                            int64_t &remainingMs)
{
    std::string name = resolveActiveSourceName();
    if (name.empty())
        return false;

    // obs_get_source_by_name increments the ref count; release when done.
    obs_source_t *source = obs_get_source_by_name(name.c_str());
    if (!source)
        return false;

    bool ok = false;
    if (isMediaSource(source)) {
        int64_t duration = obs_source_media_get_duration(source);
        int64_t time = obs_source_media_get_time(source);
        if (time < 0)
            time = 0;
        elapsedMs = time;
        remainingMs = (duration > time) ? (duration - time) : 0;
        outName = name;
        ok = true;
    }

    obs_source_release(source);
    return ok;
}
