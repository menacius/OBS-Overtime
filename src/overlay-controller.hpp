#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QList>
#include "plugin-config.hpp"

class QTimer;
class OverlayWindow;
class ProjectorTracker;

// OverlayController is the heart of the plugin. It runs a periodic timer
// on the Qt UI thread that:
//   1. detects projector windows,
//   2. creates/destroys overlay widgets to match,
//   3. computes the current time text from streaming/recording/media,
//   4. repositions each overlay to follow its projector.
//
// It also listens to OBS frontend events so streaming/recording state and
// media state changes are reflected immediately.
class OverlayController : public QObject {
    Q_OBJECT
public:
    static OverlayController *instance();

    void start();
    void stop();

    // Re-applies configuration (style + enable state) to all overlays.
    void reloadConfig();

public Q_SLOTS:
    void refresh();

private:
    explicit OverlayController(QObject *parent = nullptr);
    ~OverlayController() override;

    struct DisplayValue {
        QString id;
        QString text;
        OverlayValuePlacement placement;
        bool isMediaValue = false;
        bool warningBackground = false;
    };

    QList<DisplayValue> buildDisplayValues() const;
    void clearOverlays();

    ProjectorTracker *m_tracker = nullptr;
    QTimer *m_timer = nullptr;

    // Keyed by projector window pointer + value id to its overlay.
    QHash<QString, QPointer<OverlayWindow>> m_overlays;
};

// Installed as the OBS frontend event callback.
void overlay_frontend_event(enum obs_frontend_event event, void *private_data);
