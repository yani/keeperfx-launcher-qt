#include "workshopitemwidget.h"
#include "ui_workshopitemwidget.h"

#include <QtConcurrent/qtconcurrentrun.h>
#include "apiclient.h"

WorkshopItemWidget::WorkshopItemWidget(QWidget *parent)
    : ClickableHighlightedWidget(parent)
    , ui(new Ui::WorkshopItemWidget)
{
    ui->setupUi(this);
}

WorkshopItemWidget::~WorkshopItemWidget()
{
    delete ui;
}

void WorkshopItemWidget::setTitle(QString title)
{
    ui->titleLabel->setText(title);
}

void WorkshopItemWidget::setType(QString type)
{
    ui->typeLabel->setText(type);
}

void WorkshopItemWidget::setDate(QString date)
{
    ui->dateLabel->setText(date);
}

void WorkshopItemWidget::setAuthor(QString author)
{
    ui->authorLabel->setText(author);
}

void WorkshopItemWidget::setImage(QUrl imageUrl)
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
