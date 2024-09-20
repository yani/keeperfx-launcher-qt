#include "clickablehighlightedwidget.h"

ClickableHighlightedWidget::ClickableHighlightedWidget(QWidget *parent)
    : QWidget(parent)
{
    // Set cursor to pointing hand
    // setCursor(Qt::PointingHandCursor);

    // Create overlay for highlighting
    overlay = new QWidget(this);
    overlay->setStyleSheet("background-color: rgba(0, 0, 0, 0.15);");  // Semi-transparent highlight
    overlay->hide();  // Start hidden

    // Set overlay geometry to cover the entire widget
    overlay->setGeometry(0, 0, width(), height());
    overlay->raise();  // Ensure it's on top of child widgets
}
