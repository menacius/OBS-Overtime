#include "overlay-controller.hpp"
#include "overlay-window.hpp"
#include "projector-tracker.hpp"
#include "media-manager.hpp"
#include "plugin-config.hpp"
#include "time-utils.hpp"

#include <QTimer>
#include <QStringList>
#include <QList>
#include <QSet>
#include <QDateTime>

#include <obs-frontend-api.h>
#include <obs.h>
#include <media-io/video-io.h>

static const int kRefreshIntervalMs = 200;

// Returns the elapsed running time (ms) of an active output based on the
// number of total frames it has produced and the current video FPS.
// Frame-based timing is paused automatically while the output is paused,
// so this naturally reflects pause/resume without extra bookkeeping.
static int64_t output_elapsed_ms(obs_output_t *out)
{
    if (!out)
        return 0;

    int64_t frames = (int64_t)obs_output_get_total_frames(out);
    if (frames <= 0)
        return 0;

    double fps = 30.0;
    video_t *video = obs_get_video();
    if (video) {
        const struct video_output_info *voi =
            video_output_get_info(video);
        if (voi && voi->fps_den > 0)
            fps = (double)voi->fps_num / (double)voi->fps_den;
    }
    if (fps <= 0.0)
        fps = 30.0;

    return (int64_t)((frames / fps) * 1000.0);
}

OverlayController *OverlayController::instance()
{
    static OverlayController *ctrl = new OverlayController();
    return ctrl;
}

OverlayController::OverlayController(QObject *parent) : QObject(parent)
{
    m_tracker = new ProjectorTracker(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(kRefreshIntervalMs);
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
}

OverlayController::~OverlayController()
{
    clearOverlays();
}

void OverlayController::start()
{
    if (!m_timer->isActive())
        m_timer->start();
    refresh();
}

void OverlayController::stop()
{
    m_timer->stop();
    clearOverlays();
}

void OverlayController::reloadConfig()
{
    const PluginConfig &cfg = PluginConfig::instance();
    for (auto &overlay : m_overlays) {
        if (overlay)
            overlay->applyStyle(cfg);
    }
    if (!cfg.enabled)
        clearOverlays();
    refresh();
}

void OverlayController::clearOverlays()
{
    for (auto &overlay : m_overlays) {
        if (overlay)
            overlay->deleteLater();
    }
    m_overlays.clear();
}

QList<OverlayController::DisplayValue> OverlayController::buildDisplayValues() const
{
    const PluginConfig &cfg = PluginConfig::instance();
    QList<DisplayValue> values;

    // Streaming duration (derived from the active streaming output).
    if (cfg.showStreaming && obs_frontend_streaming_active()) {
        obs_output_t *out = obs_frontend_get_streaming_output();
        int64_t ms = output_elapsed_ms(out);
        if (out)
            obs_output_release(out);
        values.push_back({QStringLiteral("streaming"),
                          QStringLiteral("STREAM  %1")
                              .arg(QString::fromStdString(formatDuration(ms))),
                          cfg.streamingPlacement,
                          false,
                          false});
    }

    // Recording duration (derived from the active recording output).
    if (cfg.showRecording && obs_frontend_recording_active()) {
        obs_output_t *out = obs_frontend_get_recording_output();
        int64_t ms = output_elapsed_ms(out);
        if (out)
            obs_output_release(out);
        values.push_back({QStringLiteral("recording"),
                          QStringLiteral("REC  %1")
                              .arg(QString::fromStdString(formatDuration(ms))),
                          cfg.recordingPlacement,
                          false,
                          false});
    }

    // Media elapsed / remaining.
    if (cfg.showMediaElapsed || cfg.showMediaRemaining) {
        std::string name;
        int64_t elapsed = 0, remaining = 0;
        if (MediaManager::getTimes(name, elapsed, remaining)) {
            const bool warningEnabled = cfg.mediaWarningThresholdSeconds > 0 &&
                remaining <= (int64_t)cfg.mediaWarningThresholdSeconds * 1000;
            const bool warningBlinkOn = warningEnabled &&
                ((QDateTime::currentSecsSinceEpoch() % 2) == 0);

            if (cfg.showMediaElapsed) {
                values.push_back({QStringLiteral("media_elapsed"),
                                  QStringLiteral("MEDIA  %1")
                                      .arg(QString::fromStdString(
                                          formatDuration(elapsed))),
                                  cfg.mediaElapsedPlacement,
                                  true,
                                  warningBlinkOn});
            }
            if (cfg.showMediaRemaining) {
                values.push_back({QStringLiteral("media_remaining"),
                                  QStringLiteral("LEFT  -%1")
                                      .arg(QString::fromStdString(
                                          formatDuration(remaining))),
                                  cfg.mediaRemainingPlacement,
                                  true,
                                  warningBlinkOn});
            }
        }
    }

    return values;
}

void OverlayController::refresh()
{
    const PluginConfig &cfg = PluginConfig::instance();

    if (!cfg.enabled) {
        clearOverlays();
        return;
    }

    // Detect currently present projector windows.
    const QList<ProjectorInfo> projectors = m_tracker->detectProjectors();

    const QList<DisplayValue> values = buildDisplayValues();
    QSet<QString> seen;

    for (const ProjectorInfo &proj : projectors) {
        if (!proj.window || !proj.visible)
            continue;
        if (!cfg.isProjectorTargeted(proj.name.toStdString()))
            continue;

        const quintptr projectorKey = (quintptr)proj.window.data();

        for (const DisplayValue &value : values) {
            const QString key = QString::number(projectorKey, 16) +
                                QStringLiteral(":") + value.id;
            seen.insert(key);

            OverlayWindow *overlay = m_overlays.value(key).data();
            if (!overlay) {
                overlay = new OverlayWindow();
                overlay->applyStyle(cfg);
                m_overlays.insert(key, overlay);
            }

            overlay->setText(value.text);
            overlay->setBackgroundOverride(value.warningBackground,
                                           cfg.mediaWarningBackgroundColor,
                                           cfg.mediaWarningBackgroundOpacity);
            overlay->positionWithin(proj.geometry, value.placement);
            if (!overlay->isVisible())
                overlay->show();
            overlay->raise();
        }
    }

    // Remove overlays whose projector/value no longer exists (safe destruction).
    for (auto it = m_overlays.begin(); it != m_overlays.end();) {
        if (!seen.contains(it.key()) || !it.value()) {
            if (it.value())
                it.value()->deleteLater();
            it = m_overlays.erase(it);
        } else {
            ++it;
        }
    }
}

void overlay_frontend_event(enum obs_frontend_event event, void *)
{
    switch (event) {
    case OBS_FRONTEND_EVENT_STREAMING_STARTED:
    case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
    case OBS_FRONTEND_EVENT_RECORDING_STARTED:
    case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
    case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
    case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
    case OBS_FRONTEND_EVENT_SCENE_CHANGED:
        // Force an immediate refresh so state changes show instantly.
        QMetaObject::invokeMethod(OverlayController::instance(), "refresh",
                                  Qt::QueuedConnection);
        break;
    case OBS_FRONTEND_EVENT_EXIT:
        OverlayController::instance()->stop();
        break;
    default:
        break;
    }
}
