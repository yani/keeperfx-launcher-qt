#pragma once

#include <QDialog>
#include <QStringListModel>

namespace Ui {
class FileRemoverDialog;
}

class FileRemoverDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileRemoverDialog(QWidget *parent, QStringList &list);
    ~FileRemoverDialog();

private slots:
    void on_cancelButton_clicked();
    void on_removeButton_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::FileRemoverDialog *ui;
    QStringListModel *model; // Make it a member
    QStringList &list;
};
