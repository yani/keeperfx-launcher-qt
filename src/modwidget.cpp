#include "modwidget.h"
#include "ui_modwidget.h"

ModWidget::ModWidget(Mod *mod, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModWidget)
{
    ui->setupUi(this);

    ui->titleLabel->setText(mod->toString());
}

ModWidget::~ModWidget()
{
    delete ui;
}
