#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "version.h"
#include "kfxversion.h"
#include "popupsignalcombobox.h"

#include <QScreen>
#include <QEvent>
#include <QMouseEvent>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // Make sure this widget starts at the first tab
    // When working in Qt's UI editor you can change it to another tab by accident
    ui->tabWidget->setCurrentIndex(0);

    // Set versions in about label
    ui->labelAbout->setText(ui->labelAbout->text()
                                .replace("<kfx_version>", KfxVersion::currentVersion.string)
                                .replace("<launcher_version>", LAUNCHER_VERSION));



    // Handle the 'display monitor' dropdown
    setupDisplayMonitorDropdown();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setupDisplayMonitorDropdown()
{
    // Get the placeholder combo box
    QComboBox *oldComboBox = ui->comboBoxDisplayMonitor;
    QRect geometry = oldComboBox->geometry();
    QWidget *parentWidget = oldComboBox->parentWidget();
    delete ui->comboBoxDisplayMonitor;

    // Create the combobox that handles the popup signal
    // This signal is used to show and hide monitor display numers
    PopupSignalComboBox *popupComboBoxMonitorDisplay = new PopupSignalComboBox(parentWidget);
    popupComboBoxMonitorDisplay->setGeometry(geometry);

    // Get the list of available screens
    QList<QScreen *> screens = QGuiApplication::screens();

    // Loop through each screen and print index and name
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *screen = screens.at(i);

        int index = i;
        int number = i + 1;
        int roundedRefreshRate = qRound(screen->refreshRate());

        QString name = QString::asprintf("#%d %s %dhz - %dx%d",
                                         number,
                                         screen->model().toUtf8().constData(),
                                         roundedRefreshRate,
                                         screen->geometry().width(),
                                         screen->geometry().height());

        popupComboBoxMonitorDisplay->addItem(name);
    }

    // Connect slots to show and hide monitor overlays
    //connect(ui->comboBoxDisplayMonitor, &QComboBox::activated, this, &SettingsDialog::showMonitorNumberOverlays);
    QObject::connect(popupComboBoxMonitorDisplay, &PopupSignalComboBox::popupOpened, this, &SettingsDialog::showMonitorNumberOverlays);
    QObject::connect(popupComboBoxMonitorDisplay, &PopupSignalComboBox::popupClosed, this, &SettingsDialog::hideMonitorNumberOverlays);
}

void SettingsDialog::showMonitorNumberOverlays()
{
    qDebug() << "Show monitor number overlays";

    // We can't show the overlays on wayland
    /*if (QGuiApplication::platformName() == "wayland") {
        qDebug() << "Monitor overlays not shown because running under 'wayland'";
        return;
    }*/

    // Get the list of all screens
    QList<QScreen *> screens = QGuiApplication::screens();

    // Create a label on each screen at the top-right corner
    for (int i = 0; i < screens.size(); ++i) {

        QScreen *screen = screens[i];

        // Create the overlay widget
        QWidget *overlay = new QWidget;
        overlay->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                                | Qt::BypassWindowManagerHint | Qt::X11BypassWindowManagerHint);
        //overlay->setAttribute(Qt::WA_TranslucentBackground);
        overlay->setScreen(screen);

        // Set background color to black with some transparency
        QPalette palette = overlay->palette();
        palette.setColor(QPalette::Window, QColor(0, 0, 0));
        overlay->setAutoFillBackground(true);
        overlay->setPalette(palette);

        // Set size and position to top-right of the respective screen
        QRect screenGeometry = screen->geometry();
        overlay->setGeometry(screenGeometry.right() - 120, screenGeometry.top() + 20, 100, 100);

        // Create label showing the screen number
        QLabel *label = new QLabel(QString::number(i + 1), overlay);
        label->setStyleSheet("color: white; font-size: 40px;"); // Set text color and size
        label->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(overlay);
        layout->addWidget(label);
        overlay->setLayout(layout);

        overlay->show();
        monitorNumberOverlays.append(overlay); // Keep track of the overlays to remove later

        overlay->setScreen(screen);
    }
}

void SettingsDialog::hideMonitorNumberOverlays()
{
    qDebug() << "Hide monitor number overlays";

    // Remove all the overlays
    for (QWidget *overlay : monitorNumberOverlays) {
        overlay->hide();
        delete overlay;
    }
    monitorNumberOverlays.clear();
}
