#pragma once

#include <QDialog>

namespace Ui {
class DirectConnectDialog;
}

class DirectConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DirectConnectDialog(QWidget *parent = nullptr);
    ~DirectConnectDialog();

    QString getIp();
    int getPort();

private slots:
    void on_sendButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::DirectConnectDialog *ui;

    QString ip;
    int port;
};
