#pragma once

#include "clickablehighlightedwidget.h"

#include <QWidget>

namespace Ui {
class WorkshopItemWidget;
}

class WorkshopItemWidget : public ClickableHighlightedWidget
{
    Q_OBJECT

public:
    explicit WorkshopItemWidget(QWidget *parent = nullptr);
    ~WorkshopItemWidget();

    void setTitle(QString title);
    void setType(QString type);
    void setDate(QString date);
    void setAuthor(QString author);

    void setImage(QUrl imageUrl);

private:
    Ui::WorkshopItemWidget *ui;
};
