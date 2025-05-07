#include "newsarticlewidget.h"
#include "ui_newsarticlewidget.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QPainter>

#include "apiclient.h"
#include "imagehelper.h"
#include "launcheroptions.h"

NewsArticleWidget::NewsArticleWidget(QWidget *parent)
    : ClickableHighlightedWidget(parent)
    , ui(new Ui::NewsArticleWidget)
{
    ui->setupUi(this);
}

NewsArticleWidget::~NewsArticleWidget()
{
    delete ui;
}

void NewsArticleWidget::setTitle(QString title)
{
    ui->titleLabel->setText(title);
}

void NewsArticleWidget::setDate(QString date)
{
    ui->dateLabel->setText(date);
}

void NewsArticleWidget::setExcerpt(QString excerpt)
{
    // Use fontMetrics() to elide (shorten) the text with ellipsis
    // TODO: make the width dynamic
    QString elidedText = ui->excerptLabel->fontMetrics().elidedText(excerpt, Qt::ElideRight, 375);

    //ui->excerptLabel->setText(elidedText);

    ui->excerptLabel->setText(excerpt);
}

void NewsArticleWidget::setImagePixmap(QPixmap pixmap)
{
    // Create image label
    QLabel *imageLabel = new QLabel(ui->frame);
    imageLabel->setPixmap(pixmap);
}
