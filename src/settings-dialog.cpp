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


void SettingsDialog::addPositionItems(QComboBox *combo)
{
    combo->addItem(obs_module_text("Position.Top"), "top");
    combo->addItem(obs_module_text("Position.Bottom"), "bottom");
    combo->addItem(obs_module_text("Position.Left"), "left");
    combo->addItem(obs_module_text("Position.Right"), "right");
    combo->addItem(obs_module_text("Position.TopLeft"), "top-left");
    combo->addItem(obs_module_text("Position.TopRight"), "top-right");
    combo->addItem(obs_module_text("Position.BottomLeft"), "bottom-left");
    combo->addItem(obs_module_text("Position.BottomRight"), "bottom-right");
    combo->addItem(obs_module_text("Position.Center"), "center");
}

void SettingsDialog::setPlacementControls(QComboBox *positionCombo,
                                          QSpinBox *offsetSpin,
                                          const OverlayValuePlacement &placement)
{
    const int idx = positionCombo->findData(overlayPositionToString(placement.position));
    if (idx >= 0)
        positionCombo->setCurrentIndex(idx);
    offsetSpin->setValue(placement.edgeOffset);
}

void SettingsDialog::readPlacementControls(QComboBox *positionCombo,
                                           QSpinBox *offsetSpin,
                                           OverlayValuePlacement &placement) const
{
    placement.position = overlayPositionFromString(
        positionCombo->currentData().toString().toUtf8().constData());
    placement.edgeOffset = offsetSpin->value();
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
    m_mediaTimesOnlyWhenActivePlaying = new QCheckBox(
        obs_module_text("Settings.MediaOnlyWhenActivePlaying"), valuesGroup);
    valuesLayout->addWidget(m_showMediaElapsed);
    valuesLayout->addWidget(m_showMediaRemaining);
    valuesLayout->addWidget(m_mediaTimesOnlyWhenActivePlaying);
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
    QObject::connect(m_textColorBtn, SIGNAL(clicked()), this, SLOT(pickTextColor()));
    QObject::connect(m_bgColorBtn, SIGNAL(clicked()), this, SLOT(pickBackgroundColor()));

    // Media end warning group
    auto *mediaWarningGroup = new QGroupBox(
        obs_module_text("Settings.Group.MediaWarning"), this);
    auto *mediaWarningForm = new QFormLayout(mediaWarningGroup);
    m_mediaWarningThreshold = new QSpinBox(mediaWarningGroup);
    m_mediaWarningThreshold->setRange(0, 86400);
    m_mediaWarningThreshold->setSuffix(QStringLiteral(" s"));
    m_mediaWarningBgColorBtn = new QPushButton(mediaWarningGroup);
    m_mediaWarningBgOpacity = new QSlider(Qt::Horizontal, mediaWarningGroup);
    m_mediaWarningBgOpacity->setRange(0, 255);
    mediaWarningForm->addRow(obs_module_text("Settings.MediaWarningThreshold"),
                             m_mediaWarningThreshold);
    mediaWarningForm->addRow(obs_module_text("Settings.MediaWarningBackgroundColor"),
                             m_mediaWarningBgColorBtn);
    mediaWarningForm->addRow(obs_module_text("Settings.MediaWarningBackgroundOpacity"),
                             m_mediaWarningBgOpacity);
    root->addWidget(mediaWarningGroup);
    QObject::connect(m_mediaWarningBgColorBtn, SIGNAL(clicked()), this,
                     SLOT(pickMediaWarningBackgroundColor()));

    // Per-value position group
    auto *posGroup = new QGroupBox(obs_module_text("Settings.Group.ValuePositions"), this);
    auto *posForm = new QFormLayout(posGroup);

    m_streamingPosition = new QComboBox(posGroup);
    addPositionItems(m_streamingPosition);
    m_streamingOffset = new QSpinBox(posGroup);
    m_streamingOffset->setRange(0, 2000);
    posForm->addRow(obs_module_text("Settings.StreamingPosition"), m_streamingPosition);
    posForm->addRow(obs_module_text("Settings.StreamingOffset"), m_streamingOffset);

    m_recordingPosition = new QComboBox(posGroup);
    addPositionItems(m_recordingPosition);
    m_recordingOffset = new QSpinBox(posGroup);
    m_recordingOffset->setRange(0, 2000);
    posForm->addRow(obs_module_text("Settings.RecordingPosition"), m_recordingPosition);
    posForm->addRow(obs_module_text("Settings.RecordingOffset"), m_recordingOffset);

    m_mediaElapsedPosition = new QComboBox(posGroup);
    addPositionItems(m_mediaElapsedPosition);
    m_mediaElapsedOffset = new QSpinBox(posGroup);
    m_mediaElapsedOffset->setRange(0, 2000);
    posForm->addRow(obs_module_text("Settings.MediaElapsedPosition"), m_mediaElapsedPosition);
    posForm->addRow(obs_module_text("Settings.MediaElapsedOffset"), m_mediaElapsedOffset);

    m_mediaRemainingPosition = new QComboBox(posGroup);
    addPositionItems(m_mediaRemainingPosition);
    m_mediaRemainingOffset = new QSpinBox(posGroup);
    m_mediaRemainingOffset->setRange(0, 2000);
    posForm->addRow(obs_module_text("Settings.MediaRemainingPosition"), m_mediaRemainingPosition);
    posForm->addRow(obs_module_text("Settings.MediaRemainingOffset"), m_mediaRemainingOffset);

    root->addWidget(posGroup);

    // Projectors group
    auto *projGroup = new QGroupBox(obs_module_text("Settings.Group.Projectors"), this);
    auto *projLayout = new QVBoxLayout(projGroup);
    m_projectorList = new QListWidget(projGroup);
    auto *refreshBtn = new QPushButton("Refresh", projGroup);
    projLayout->addWidget(m_projectorList);
    projLayout->addWidget(refreshBtn);
    root->addWidget(projGroup);
    QObject::connect(refreshBtn, SIGNAL(clicked()), this, SLOT(refreshProjectorList()));

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);
    QObject::connect(buttons, SIGNAL(accepted()), this, SLOT(onAccept()));
    QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

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
    m_mediaTimesOnlyWhenActivePlaying->setChecked(
        cfg.mediaTimesOnlyWhenActivePlaying);

    m_textColor = cfg.textColor;
    m_bgColor = cfg.backgroundColor;
    updateColorButton(m_textColorBtn, m_textColor);
    updateColorButton(m_bgColorBtn, m_bgColor);
    m_bgOpacity->setValue(cfg.backgroundOpacity);
    m_bgPadding->setValue(cfg.backgroundPadding);

    m_mediaWarningThreshold->setValue(cfg.mediaWarningThresholdSeconds);
    m_mediaWarningBgColor = cfg.mediaWarningBackgroundColor;
    updateColorButton(m_mediaWarningBgColorBtn, m_mediaWarningBgColor);
    m_mediaWarningBgOpacity->setValue(cfg.mediaWarningBackgroundOpacity);

    setPlacementControls(m_streamingPosition, m_streamingOffset, cfg.streamingPlacement);
    setPlacementControls(m_recordingPosition, m_recordingOffset, cfg.recordingPlacement);
    setPlacementControls(m_mediaElapsedPosition, m_mediaElapsedOffset, cfg.mediaElapsedPlacement);
    setPlacementControls(m_mediaRemainingPosition, m_mediaRemainingOffset, cfg.mediaRemainingPlacement);
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

void SettingsDialog::pickMediaWarningBackgroundColor()
{
    QColor c = QColorDialog::getColor(argbToColor(m_mediaWarningBgColor), this,
                                      "Media warning background color",
                                      QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_mediaWarningBgColor = colorToArgb(c);
        updateColorButton(m_mediaWarningBgColorBtn, m_mediaWarningBgColor);
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
    cfg.mediaTimesOnlyWhenActivePlaying =
        m_mediaTimesOnlyWhenActivePlaying->isChecked();
    cfg.mediaWarningThresholdSeconds = m_mediaWarningThreshold->value();
    cfg.mediaWarningBackgroundColor = m_mediaWarningBgColor;
    cfg.mediaWarningBackgroundOpacity = m_mediaWarningBgOpacity->value();
    cfg.textColor = m_textColor;
    cfg.backgroundColor = m_bgColor;
    cfg.backgroundOpacity = m_bgOpacity->value();
    cfg.backgroundPadding = m_bgPadding->value();
    readPlacementControls(m_streamingPosition, m_streamingOffset, cfg.streamingPlacement);
    readPlacementControls(m_recordingPosition, m_recordingOffset, cfg.recordingPlacement);
    readPlacementControls(m_mediaElapsedPosition, m_mediaElapsedOffset, cfg.mediaElapsedPlacement);
    readPlacementControls(m_mediaRemainingPosition, m_mediaRemainingOffset, cfg.mediaRemainingPlacement);

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
