#pragma once

#include <QDialog>

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
    void refreshProjectorList();

private:
    void loadFromConfig();
    void updateColorButton(QPushButton *button, unsigned int argb);

    QCheckBox *m_enabled = nullptr;
    QFontComboBox *m_fontFamily = nullptr;
    QSpinBox *m_fontSize = nullptr;

    QCheckBox *m_showStreaming = nullptr;
    QCheckBox *m_showRecording = nullptr;
    QCheckBox *m_showMediaElapsed = nullptr;
    QCheckBox *m_showMediaRemaining = nullptr;

    QPushButton *m_textColorBtn = nullptr;
    QPushButton *m_bgColorBtn = nullptr;
    QSlider *m_bgOpacity = nullptr;
    QSpinBox *m_bgPadding = nullptr;

    QComboBox *m_position = nullptr;
    QSpinBox *m_offset = nullptr;

    QListWidget *m_projectorList = nullptr;

    unsigned int m_textColor = 0xFFFFFFFF;
    unsigned int m_bgColor = 0xFF000000;
};
