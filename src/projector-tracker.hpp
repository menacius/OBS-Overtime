#pragma once

#include <QObject>
#include <QRect>
#include <QString>
#include <QList>
#include <QPointer>

class QWidget;

// Describes a single detected OBS projector window.
struct ProjectorInfo {
    QString name;        // human-readable identifier (window title)
    QPointer<QWidget> window;
    QRect geometry;      // global screen geometry
    bool visible = false;
};

// ProjectorTracker locates OBS projector windows among the top-level Qt
// widgets and reports their current geometry. OBS projectors are Qt
// windows whose class name is "OBSProjector". The tracker never holds a
// raw pointer beyond a QPointer so destroyed projectors are handled
// safely.
class ProjectorTracker : public QObject {
    Q_OBJECT
public:
    explicit ProjectorTracker(QObject *parent = nullptr);

    // Re-scans the top-level widgets for projector windows.
    QList<ProjectorInfo> detectProjectors() const;
};
