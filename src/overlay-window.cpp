#include "overlay-window.hpp"

#include <QPainter>
#include <QFontMetrics>
#include <QPaintEvent>

static QColor fromArgb(uint32_t argb)
{
    return QColor((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF,
                  (argb >> 24) & 0xFF);
}

OverlayWindow::OverlayWindow(QWidget *parent) : QWidget(parent)
{
    // Frameless, always on top, no taskbar entry, transparent background,
    // and transparent to mouse events so it never steals input from the
    // projector underneath.
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::Tool | Qt::WindowTransparentForInput |
                   Qt::BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
}

void OverlayWindow::setText(const QString &text)
{
    if (m_text == text)
        return;
    m_text = text;
    update();
}

void OverlayWindow::applyStyle(const PluginConfig &cfg)
{
    m_font = QFont(QString::fromStdString(cfg.fontFamily), cfg.fontSize);
    m_textColor = fromArgb(cfg.textColor);

    QColor bg = fromArgb(cfg.backgroundColor);
    // Background opacity overrides the alpha channel of the bg color.
    bg.setAlpha(qBound(0, cfg.backgroundOpacity, 255));
    m_bgColor = bg;
    m_padding = qMax(0, cfg.backgroundPadding);
    update();
}

void OverlayWindow::setBackgroundOverride(bool enabled, unsigned int argb,
                                          int opacity)
{
    QColor bg = fromArgb(argb);
    bg.setAlpha(qBound(0, opacity, 255));

    if (m_hasBackgroundOverride == enabled && m_overrideBgColor == bg)
        return;

    m_hasBackgroundOverride = enabled;
    m_overrideBgColor = bg;
    update();
}

QSize OverlayWindow::contentSize() const
{
    QFontMetrics fm(m_font);
    QStringList lines = m_text.split('\n');
    int width = 0;
    for (const QString &line : lines)
        width = qMax(width, fm.horizontalAdvance(line));
    int height = fm.height() * qMax(1, (int)lines.size());
    return QSize(width + m_padding * 2, height + m_padding * 2);
}

void OverlayWindow::positionWithin(const QRect &projectorRect,
                                   const OverlayValuePlacement &placement)
{
    const QSize sz = contentSize();
    const int off = qMax(0, placement.edgeOffset);

    int x = projectorRect.x();
    int y = projectorRect.y();
    const int w = projectorRect.width();
    const int h = projectorRect.height();

    const int cx = x + (w - sz.width()) / 2;
    const int cy = y + (h - sz.height()) / 2;
    const int leftX = x + off;
    const int rightX = x + w - sz.width() - off;
    const int topY = y + off;
    const int bottomY = y + h - sz.height() - off;

    switch (placement.position) {
    case OverlayPosition::Top:         x = cx;     y = topY;    break;
    case OverlayPosition::Bottom:      x = cx;     y = bottomY; break;
    case OverlayPosition::Left:        x = leftX;  y = cy;      break;
    case OverlayPosition::Right:       x = rightX; y = cy;      break;
    case OverlayPosition::TopLeft:     x = leftX;  y = topY;    break;
    case OverlayPosition::TopRight:    x = rightX; y = topY;    break;
    case OverlayPosition::BottomLeft:  x = leftX;  y = bottomY; break;
    case OverlayPosition::BottomRight: x = rightX; y = bottomY; break;
    case OverlayPosition::Center:      x = cx;     y = cy;      break;
    }

    setGeometry(x, y, sz.width(), sz.height());
}

void OverlayWindow::paintEvent(QPaintEvent *)
{
    if (m_text.isEmpty())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Rounded background panel.
    QRectF panel = rect();
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_hasBackgroundOverride ? m_overrideBgColor : m_bgColor);
    painter.drawRoundedRect(panel, 6.0, 6.0);

    // Text.
    painter.setFont(m_font);
    painter.setPen(m_textColor);
    QRectF textRect = panel.adjusted(m_padding, m_padding,
                                     -m_padding, -m_padding);
    painter.drawText(textRect, Qt::AlignCenter, m_text);
}
