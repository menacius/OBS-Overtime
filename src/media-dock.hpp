#pragma once

#include <QWidget>

class QListWidget;
class QLabel;
class QTimer;

// Dockable widget letting the user choose which media sources are
// considered for elapsed/remaining time. Lists all media-capable sources
// with checkboxes, shows the currently detected active source and its
// time, and refreshes itself safely on a timer so source add/remove/
// rename is handled without crashes.
class MediaDock : public QWidget {
    Q_OBJECT
public:
    explicit MediaDock(QWidget *parent = nullptr);

private Q_SLOTS:
    void refreshList();
    void updateStatus();
    void onItemChanged(class QListWidgetItem *item);

private:
    QListWidget *m_list = nullptr;
    QLabel *m_status = nullptr;
    QTimer *m_timer = nullptr;
    bool m_updating = false;
};
