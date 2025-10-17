#include "modwidget.h"
#include "ui_modwidget.h"
#include <qpainter.h>

ModWidget::ModWidget(Mod *mod, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModWidget)
{
    ui->setupUi(this);

    // Load name
    if (mod->nameTranslated.isEmpty() == false) {
        ui->titleLabel->setText(mod->nameTranslated);
    } else if (mod->name.isEmpty() == false) {
        ui->titleLabel->setText(mod->name);
    } else {
        ui->titleLabel->setText(mod->toString());
    }

    // Load description
    if (mod->nameTranslated.isEmpty() == false) {
        ui->descriptionLabel->setText(mod->descriptionTranslated);
    } else if (mod->name.isEmpty() == false) {
        ui->descriptionLabel->setText(mod->description);
    } else {
        ui->descriptionLabel->hide();
    }

    // Thumbnail
    if (mod->thumbnailPixmap.isNull() == false) {
        // Get the size for the thumbnail
        QSize thumbnailSize = ui->frame->frameSize();

        // Scale the source pixmap while keeping aspect ratio
        QPixmap scaledPixmap = mod->thumbnailPixmap.scaled(thumbnailSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        // Create final pixmap with transparent (or solid) background
        QPixmap finalPixmap(thumbnailSize);
        finalPixmap.fill(Qt::transparent);

        // Paint scaled image centered into finalPixmap
        QPainter painter(&finalPixmap);
        int x = (thumbnailSize.width() - scaledPixmap.width()) / 2;
        int y = (thumbnailSize.height() - scaledPixmap.height()) / 2;
        painter.drawPixmap(x, y, scaledPixmap);
        painter.end();

        // Create label and set final pixmap
        QLabel *imageLabel = new QLabel(ui->frame);
        imageLabel->setPixmap(finalPixmap);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setScaledContents(false); // prevent distorting
        imageLabel->setFixedSize(thumbnailSize);
    }
}

ModWidget::~ModWidget()
{
    delete ui;
}
