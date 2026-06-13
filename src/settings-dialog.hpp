#pragma once

#include <QDialog>

#include "plugin-config.hpp"

class QComboBox;
class QSpinBox;
class QCheckBox;
class QFontComboBox;
class QPushButton;
class QListWidget;
class QSlider;

// Modal settings dialog exposing every configurable option, grouped into
// QGroupBox sections for clarity. On accept it writes back to PluginConfig
// and persists it, then asks the controller to reload.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private Q_SLOTS:
    void onAccept();
    void pickTextColor();
    void pickBackgroundColor();
    void pickMediaWarningBackgroundColor();
    void refreshProjectorList();

private:
    void loadFromConfig();
    void updateColorButton(QPushButton *button, unsigned int argb);
    void addPositionItems(QComboBox *combo);
    void setPlacementControls(QComboBox *positionCombo, QSpinBox *offsetSpin,
                              const OverlayValuePlacement &placement);
    void readPlacementControls(QComboBox *positionCombo, QSpinBox *offsetSpin,
                               OverlayValuePlacement &placement) const;

    QCheckBox *m_enabled = nullptr;
    QFontComboBox *m_fontFamily = nullptr;
    QSpinBox *m_fontSize = nullptr;

    QCheckBox *m_showStreaming = nullptr;
    QCheckBox *m_showRecording = nullptr;
    QCheckBox *m_showMediaElapsed = nullptr;
    QCheckBox *m_showMediaRemaining = nullptr;
    QCheckBox *m_mediaTimesOnlyWhenActivePlaying = nullptr;

    QPushButton *m_textColorBtn = nullptr;
    QPushButton *m_bgColorBtn = nullptr;
    QSlider *m_bgOpacity = nullptr;
    QSpinBox *m_bgPadding = nullptr;

    QComboBox *m_streamingPosition = nullptr;
    QSpinBox *m_streamingOffset = nullptr;
    QComboBox *m_recordingPosition = nullptr;
    QSpinBox *m_recordingOffset = nullptr;
    QComboBox *m_mediaElapsedPosition = nullptr;
    QSpinBox *m_mediaElapsedOffset = nullptr;
    QComboBox *m_mediaRemainingPosition = nullptr;
    QSpinBox *m_mediaRemainingOffset = nullptr;

    QSpinBox *m_mediaWarningThreshold = nullptr;
    QPushButton *m_mediaWarningBgColorBtn = nullptr;
    QSlider *m_mediaWarningBgOpacity = nullptr;

    QListWidget *m_projectorList = nullptr;

    unsigned int m_textColor = 0xFFFFFFFF;
    unsigned int m_bgColor = 0xFF000000;
    unsigned int m_mediaWarningBgColor = 0xFFFF0000;
};
