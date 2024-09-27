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

        // For some reason this is triggered when the widget is hidden (by a tab switch for example)
        // It shouldn't really be an issue but it should be fixed at some point

        emit popupClosed();      // Emit custom signal
        QComboBox::hidePopup();  // Call the base class implementation
    }
};
