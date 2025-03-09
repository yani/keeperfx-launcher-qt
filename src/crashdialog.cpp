#include "crashdialog.h"
#include "ui_crashdialog.h"

CrashDialog::CrashDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CrashDialog)
{
    ui->setupUi(this);
}

CrashDialog::~CrashDialog()
{
    delete ui;
}
