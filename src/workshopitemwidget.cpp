#include "workshopitemwidget.h"
#include "ui_workshopitemwidget.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QPainter>

#include "apiclient.h"
#include "imagehelper.h"
#include "launcheroptions.h"

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
    // Get target size of widget
    QSize targetSize(ui->frame->width(), ui->frame->height());

    // Create image label
    QLabel *imageLabel = new QLabel(ui->frame);
    imageLabel->setPixmap(ImageHelper::getOnlineScaledPixmap(imageUrl, targetSize));
}
