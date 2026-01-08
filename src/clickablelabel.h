#pragma once

#include <QColor>
#include <QEnterEvent>
#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    ClickableLabel(QWidget* parent = nullptr)
        : QLabel(parent)
    {
        setAttribute(Qt::WA_Hover, true);
    }

    void setBaseColor(const QColor& color)
    {
        baseColor = color;
        hoverColor = color.lighter(120); // brighten by 20%
        setStyleSheet(QString("color: %1;").arg(baseColor.name()));
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }

        QLabel::mousePressEvent(event);
    }

    void enterEvent(QEnterEvent* event) override
    {
        setStyleSheet(QString("color: %1;").arg(hoverColor.name()));
        QLabel::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        setStyleSheet(QString("color: %1;").arg(baseColor.name()));
        QLabel::leaveEvent(event);
    }

private:
    QColor baseColor = QColor(0x999999); // #999999
    QColor hoverColor = baseColor.lighter(120);
};
