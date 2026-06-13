#include "projector-tracker.hpp"

#include <QApplication>
#include <QWidget>
#include <QWindow>

ProjectorTracker::ProjectorTracker(QObject *parent) : QObject(parent) {}

QList<ProjectorInfo> ProjectorTracker::detectProjectors() const
{
    QList<ProjectorInfo> result;

    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevels) {
        if (!widget)
            continue;

        // OBS creates projector windows using the OBSProjector class.
        const QString className = widget->metaObject()->className();
        const bool isProjector =
            className.contains("OBSProjector", Qt::CaseInsensitive) ||
            widget->objectName().contains("Projector", Qt::CaseInsensitive);
        if (!isProjector)
            continue;

        ProjectorInfo info;
        info.window = widget;
        info.visible = widget->isVisible();
        info.geometry = QRect(widget->mapToGlobal(QPoint(0, 0)),
                              widget->size());

        QString title = widget->windowTitle();
        if (title.isEmpty())
            title = widget->objectName();
        if (title.isEmpty())
            title = QStringLiteral("Projector %1")
                        .arg((quintptr)widget, 0, 16);
        info.name = title;

        result.append(info);
    }

    return result;
}
