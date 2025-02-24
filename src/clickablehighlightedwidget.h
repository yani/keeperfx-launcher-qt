#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QResizeEvent>
#include <QDesktopServices>

class ClickableHighlightedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClickableHighlightedWidget(QWidget *parent = nullptr);

    void setTargetUrl(QString url){
        targetUrl = QUrl(url);
    }

protected:
    QWidget *overlay;  // Overlay for the highlight effect
    QUrl targetUrl;

    // Handle mouse press event
    void mousePressEvent(QMouseEvent *event) override {
        QWidget::mousePressEvent(event);  // Call base class event
        QDesktopServices::openUrl(targetUrl);
    }

    // Handle hover enter event
    void enterEvent(QEnterEvent *event) override {
        overlay->show();  // Show the overlay
        QWidget::enterEvent(event);
    }

    // Handle hover leave event
    void leaveEvent(QEvent *event) override {
        overlay->hide();  // Hide the overlay
        QWidget::leaveEvent(event);
    }

    // Handle widget resize event
    void resizeEvent(QResizeEvent *event) override {
        overlay->setGeometry(0, 0, width(), height());  // Resize overlay to cover widget
        QWidget::resizeEvent(event);
    }
};
