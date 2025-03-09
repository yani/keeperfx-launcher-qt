#pragma once

#include <QDialog>

namespace Ui {
class CrashDialog;
}

class CrashDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CrashDialog(QWidget *parent = nullptr);
    ~CrashDialog();

private:
    Ui::CrashDialog *ui;
};
