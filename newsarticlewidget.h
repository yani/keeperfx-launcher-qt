#pragma once

#include <QWidget>
#include "clickablehighlightedwidget.h"

namespace Ui {
class NewsArticleWidget;
}

class NewsArticleWidget : public ClickableHighlightedWidget
{
    Q_OBJECT

public:
    explicit NewsArticleWidget(QWidget *parent = nullptr);
    ~NewsArticleWidget();

    void setTitle(QString title);
    void setDate(QString date);
    void setExcerpt(QString author);

    void setImage(QUrl imageUrl);

private:
    Ui::NewsArticleWidget *ui;
};
