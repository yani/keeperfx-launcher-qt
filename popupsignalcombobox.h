#pragma once

#include <QComboBox>

class PopupSignalComboBox : public QComboBox {
    Q_OBJECT

public:
    explicit PopupSignalComboBox(QWidget *parent = nullptr) : QComboBox(parent) {}

signals:
    void popupOpened();
    void popupClosed();

protected:
    void showPopup() override {
        emit popupOpened();      // Emit custom signal
        QComboBox::showPopup();  // Call the base class implementation
    }

    void hidePopup() override {
        emit popupClosed();      // Emit custom signal
        QComboBox::hidePopup();  // Call the base class implementation
    }
};
