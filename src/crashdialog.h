#pragma once

#include "savefile.h"

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

private slots:
    void on_cancelButton_clicked();
    void on_sendButton_clicked();

private:
    Ui::CrashDialog *ui;
    QList<SaveFile *> saveFileList;
};
