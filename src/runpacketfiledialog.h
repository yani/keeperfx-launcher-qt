#pragma once

#include <QDialog>

namespace Ui { class RunPacketFileDialog; }

class RunPacketFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RunPacketFileDialog(QWidget *parent = nullptr);
    ~RunPacketFileDialog();

    QString getPacketFileName();

private slots:
    void on_cancelButton_clicked();
    void on_startButton_clicked();
    void updateStartButton();

private:
    Ui::RunPacketFileDialog *ui;

    QString packetFileName;
};
