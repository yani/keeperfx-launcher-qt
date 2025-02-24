#pragma once

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
class CopyDkFilesDialog;
}

class CopyDkFilesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CopyDkFilesDialog(QWidget *parent = nullptr);
    ~CopyDkFilesDialog();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_browseButton_clicked();
    void on_copyButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::CopyDkFilesDialog *ui;
};
