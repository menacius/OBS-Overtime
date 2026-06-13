#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPointer>

#include "plugin-config.hpp"

// A frameless, transparent, click-through, always-on-top widget that
// renders the formatted time text. One instance is created per tracked
// projector window. It paints itself and is positioned by the controller
// to follow the projector geometry.
class OverlayWindow : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWindow(QWidget *parent = nullptr);

    // Update the text to render. Triggers a repaint and resize.
    void setText(const QString &text);

    // Re-reads styling (font/colors/padding) from the config.
    void applyStyle(const PluginConfig &cfg);

    // Enables or disables a temporary per-value background override, used for
    // media end warning blinking.
    void setBackgroundOverride(bool enabled, unsigned int argb, int opacity);

    // Computes and applies the on-screen geometry so the overlay sits at
    // the configured position within the given projector rectangle.
    void positionWithin(const QRect &projectorRect, const OverlayValuePlacement &placement);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSize contentSize() const;

    QString m_text;
    QFont m_font;
    QColor m_textColor = Qt::white;
    QColor m_bgColor = QColor(0, 0, 0, 160);
    QColor m_overrideBgColor = QColor(255, 0, 0, 200);
    bool m_hasBackgroundOverride = false;
    int m_padding = 8;
};
