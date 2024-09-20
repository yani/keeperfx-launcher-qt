#include "newsarticlewidget.h"
#include "ui_newsarticlewidget.h"

#include "apiclient.h"

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

void NewsArticleWidget::setImage(QUrl imageUrl)
{
    QImage image = ApiClient::downloadImage(imageUrl);
    if(image.isNull()){
        return;
    }

    QLabel *imageLabel = new QLabel(ui->frame);
    imageLabel->setPixmap(
        QPixmap::fromImage(image).scaled(
            ui->frame->width(),
            ui->frame->height(),
            Qt::KeepAspectRatioByExpanding,
            Qt::FastTransformation
            )
        );
}
