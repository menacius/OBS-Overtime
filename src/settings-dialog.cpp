#include "settings-dialog.hpp"
#include "plugin-config.hpp"
#include "overlay-controller.hpp"
#include "projector-tracker.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QSlider>
#include <QLabel>
#include <QColorDialog>
#include <QDialogButtonBox>

#include <obs-module.h>

static QColor argbToColor(unsigned int argb)
{
    return QColor((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF,
                  (argb >> 24) & 0xFF);
}

static unsigned int colorToArgb(const QColor &c)
{
    return ((unsigned int)c.alpha() << 24) | ((unsigned int)c.red() << 16) |
           ((unsigned int)c.green() << 8) | (unsigned int)c.blue();
}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(obs_module_text("Settings.Title"));
    setModal(true);

    auto *root = new QVBoxLayout(this);

    m_enabled = new QCheckBox(obs_module_text("Settings.Enabled"), this);
    root->addWidget(m_enabled);

    // Font group
    auto *fontGroup = new QGroupBox(obs_module_text("Settings.Group.Font"), this);
    auto *fontForm = new QFormLayout(fontGroup);
    m_fontFamily = new QFontComboBox(fontGroup);
    m_fontSize = new QSpinBox(fontGroup);
    m_fontSize->setRange(6, 256);
    fontForm->addRow(obs_module_text("Settings.FontFamily"), m_fontFamily);
    fontForm->addRow(obs_module_text("Settings.FontSize"), m_fontSize);
    root->addWidget(fontGroup);

    // Values group
    auto *valuesGroup = new QGroupBox(obs_module_text("Settings.Group.Values"), this);
    auto *valuesLayout = new QVBoxLayout(valuesGroup);
    m_showStreaming = new QCheckBox(obs_module_text("Settings.ShowStreaming"), valuesGroup);
    m_showRecording = new QCheckBox(obs_module_text("Settings.ShowRecording"), valuesGroup);
    m_showMediaElapsed = new QCheckBox(obs_module_text("Settings.ShowMediaElapsed"), valuesGroup);
    m_showMediaRemaining = new QCheckBox(obs_module_text("Settings.ShowMediaRemaining"), valuesGroup);
    valuesLayout->addWidget(m_showStreaming);
    valuesLayout->addWidget(m_showRecording);
    valuesLayout->addWidget(m_showMediaElapsed);
    valuesLayout->addWidget(m_showMediaRemaining);
    root->addWidget(valuesGroup);

    // Colors group
    auto *colorGroup = new QGroupBox(obs_module_text("Settings.Group.Colors"), this);
    auto *colorForm = new QFormLayout(colorGroup);
    m_textColorBtn = new QPushButton(colorGroup);
    m_bgColorBtn = new QPushButton(colorGroup);
    m_bgOpacity = new QSlider(Qt::Horizontal, colorGroup);
    m_bgOpacity->setRange(0, 255);
    m_bgPadding = new QSpinBox(colorGroup);
    m_bgPadding->setRange(0, 200);
    colorForm->addRow(obs_module_text("Settings.TextColor"), m_textColorBtn);
    colorForm->addRow(obs_module_text("Settings.BackgroundColor"), m_bgColorBtn);
    colorForm->addRow(obs_module_text("Settings.BackgroundOpacity"), m_bgOpacity);
    colorForm->addRow(obs_module_text("Settings.BackgroundPadding"), m_bgPadding);
    root->addWidget(colorGroup);
    connect(m_textColorBtn, &QPushButton::clicked, this, &SettingsDialog::pickTextColor);
    connect(m_bgColorBtn, &QPushButton::clicked, this, &SettingsDialog::pickBackgroundColor);

    // Position group
    auto *posGroup = new QGroupBox(obs_module_text("Settings.Group.Position"), this);
    auto *posForm = new QFormLayout(posGroup);
    m_position = new QComboBox(posGroup);
    m_position->addItem(obs_module_text("Position.Top"), "top");
    m_position->addItem(obs_module_text("Position.Bottom"), "bottom");
    m_position->addItem(obs_module_text("Position.Left"), "left");
    m_position->addItem(obs_module_text("Position.Right"), "right");
    m_position->addItem(obs_module_text("Position.TopLeft"), "top-left");
    m_position->addItem(obs_module_text("Position.TopRight"), "top-right");
    m_position->addItem(obs_module_text("Position.BottomLeft"), "bottom-left");
    m_position->addItem(obs_module_text("Position.BottomRight"), "bottom-right");
    m_position->addItem(obs_module_text("Position.Center"), "center");
    m_offset = new QSpinBox(posGroup);
    m_offset->setRange(0, 2000);
    posForm->addRow(obs_module_text("Settings.Position"), m_position);
    posForm->addRow(obs_module_text("Settings.Offset"), m_offset);
    root->addWidget(posGroup);

    // Projectors group
    auto *projGroup = new QGroupBox(obs_module_text("Settings.Group.Projectors"), this);
    auto *projLayout = new QVBoxLayout(projGroup);
    m_projectorList = new QListWidget(projGroup);
    auto *refreshBtn = new QPushButton("Refresh", projGroup);
    projLayout->addWidget(m_projectorList);
    projLayout->addWidget(refreshBtn);
    root->addWidget(projGroup);
    connect(refreshBtn, &QPushButton::clicked, this, &SettingsDialog::refreshProjectorList);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    loadFromConfig();
    refreshProjectorList();
}

void SettingsDialog::updateColorButton(QPushButton *button, unsigned int argb)
{
    QColor c = argbToColor(argb);
    button->setText(c.name(QColor::HexArgb));
    QString style = QStringLiteral(
                        "background-color: rgb(%1,%2,%3); color: %4;")
                        .arg(c.red()).arg(c.green()).arg(c.blue())
                        .arg(c.lightness() > 127 ? "black" : "white");
    button->setStyleSheet(style);
}

void SettingsDialog::loadFromConfig()
{
    const PluginConfig &cfg = PluginConfig::instance();
    m_enabled->setChecked(cfg.enabled);
    m_fontFamily->setCurrentFont(QFont(QString::fromStdString(cfg.fontFamily)));
    m_fontSize->setValue(cfg.fontSize);
    m_showStreaming->setChecked(cfg.showStreaming);
    m_showRecording->setChecked(cfg.showRecording);
    m_showMediaElapsed->setChecked(cfg.showMediaElapsed);
    m_showMediaRemaining->setChecked(cfg.showMediaRemaining);

    m_textColor = cfg.textColor;
    m_bgColor = cfg.backgroundColor;
    updateColorButton(m_textColorBtn, m_textColor);
    updateColorButton(m_bgColorBtn, m_bgColor);
    m_bgOpacity->setValue(cfg.backgroundOpacity);
    m_bgPadding->setValue(cfg.backgroundPadding);

    int idx = m_position->findData(overlayPositionToString(cfg.position));
    if (idx >= 0)
        m_position->setCurrentIndex(idx);
    m_offset->setValue(cfg.edgeOffset);
}

void SettingsDialog::pickTextColor()
{
    QColor c = QColorDialog::getColor(argbToColor(m_textColor), this,
                                      "Text color",
                                      QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_textColor = colorToArgb(c);
        updateColorButton(m_textColorBtn, m_textColor);
    }
}

void SettingsDialog::pickBackgroundColor()
{
    QColor c = QColorDialog::getColor(argbToColor(m_bgColor), this,
                                      "Background color",
                                      QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_bgColor = colorToArgb(c);
        updateColorButton(m_bgColorBtn, m_bgColor);
    }
}

void SettingsDialog::refreshProjectorList()
{
    const PluginConfig &cfg = PluginConfig::instance();
    m_projectorList->clear();

    ProjectorTracker tracker;
    const auto projectors = tracker.detectProjectors();
    for (const auto &proj : projectors) {
        auto *item = new QListWidgetItem(proj.name, m_projectorList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        bool checked = cfg.targetProjectors.empty() ||
                       cfg.isProjectorTargeted(proj.name.toStdString());
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
    if (projectors.isEmpty()) {
        auto *item = new QListWidgetItem(
            "No projector windows open", m_projectorList);
        item->setFlags(Qt::NoItemFlags);
    }
}

void SettingsDialog::onAccept()
{
    PluginConfig &cfg = PluginConfig::instance();
    cfg.enabled = m_enabled->isChecked();
    cfg.fontFamily = m_fontFamily->currentFont().family().toStdString();
    cfg.fontSize = m_fontSize->value();
    cfg.showStreaming = m_showStreaming->isChecked();
    cfg.showRecording = m_showRecording->isChecked();
    cfg.showMediaElapsed = m_showMediaElapsed->isChecked();
    cfg.showMediaRemaining = m_showMediaRemaining->isChecked();
    cfg.textColor = m_textColor;
    cfg.backgroundColor = m_bgColor;
    cfg.backgroundOpacity = m_bgOpacity->value();
    cfg.backgroundPadding = m_bgPadding->value();
    cfg.position = overlayPositionFromString(
        m_position->currentData().toString().toUtf8().constData());
    cfg.edgeOffset = m_offset->value();

    cfg.targetProjectors.clear();
    int total = 0, checked = 0;
    for (int i = 0; i < m_projectorList->count(); i++) {
        QListWidgetItem *item = m_projectorList->item(i);
        if (!(item->flags() & Qt::ItemIsUserCheckable))
            continue;
        total++;
        if (item->checkState() == Qt::Checked) {
            checked++;
            cfg.targetProjectors.push_back(item->text().toStdString());
        }
    }
    // If everything is checked, store empty == "all" so newly opened
    // projectors are also covered automatically.
    if (total > 0 && checked == total)
        cfg.targetProjectors.clear();

    cfg.save();
    OverlayController::instance()->reloadConfig();
    accept();
}
