#include "media-dock.hpp"
#include "media-manager.hpp"
#include "plugin-config.hpp"
#include "time-utils.hpp"

#include <obs-module.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include <QSet>
#include <QString>

MediaDock::MediaDock(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);

    auto *header = new QLabel(obs_module_text("Dock.Header"), this);
    header->setWordWrap(true);
    layout->addWidget(header);

    m_list = new QListWidget(this);
    layout->addWidget(m_list, 1);
    connect(m_list, &QListWidget::itemChanged, this, &MediaDock::onItemChanged);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    // Periodic refresh keeps the list and status in sync with OBS even
    // when sources are added/removed/renamed externally.
    m_timer = new QTimer(this);
    m_timer->setInterval(750);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        refreshList();
        updateStatus();
    });
    m_timer->start();

    refreshList();
    updateStatus();
}

void MediaDock::refreshList()
{
    const PluginConfig &cfg = PluginConfig::instance();
    auto sources = MediaManager::enumerateMediaSources();

    // Build the set of names currently shown to detect changes cheaply.
    QSet<QString> current;
    for (int i = 0; i < m_list->count(); i++)
        current.insert(m_list->item(i)->text());

    QSet<QString> incoming;
    for (const auto &s : sources)
        incoming.insert(QString::fromStdString(s.name));

    if (current == incoming)
        return;  // No structural change; avoid rebuilding/flicker.

    m_updating = true;
    m_list->clear();
    for (const auto &s : sources) {
        QString name = QString::fromStdString(s.name);
        auto *item = new QListWidgetItem(name, m_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(cfg.isMediaSourceEnabled(s.name) ? Qt::Checked
                                                             : Qt::Unchecked);
    }
    if (sources.empty()) {
        auto *item = new QListWidgetItem(
            obs_module_text("Dock.NoSources"), m_list);
        item->setFlags(Qt::NoItemFlags);
    }
    m_updating = false;
}

void MediaDock::onItemChanged(QListWidgetItem *item)
{
    if (m_updating || !item)
        return;
    if (!(item->flags() & Qt::ItemIsUserCheckable))
        return;

    PluginConfig &cfg = PluginConfig::instance();
    cfg.setMediaSourceEnabled(item->text().toStdString(),
                              item->checkState() == Qt::Checked);
    cfg.save();
    updateStatus();
}

void MediaDock::updateStatus()
{
    std::string name;
    int64_t elapsed = 0, remaining = 0;
    if (MediaManager::getTimes(name, elapsed, remaining)) {
        m_status->setText(
            QStringLiteral("%1 %2\n%3 / -%4")
                .arg(obs_module_text("Dock.Active"))
                .arg(QString::fromStdString(name))
                .arg(QString::fromStdString(formatDuration(elapsed)))
                .arg(QString::fromStdString(formatDuration(remaining))));
    } else {
        m_status->setText(QStringLiteral("%1 %2")
                              .arg(obs_module_text("Dock.Active"))
                              .arg(obs_module_text("Dock.None")));
    }
}
