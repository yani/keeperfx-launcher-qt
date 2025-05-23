#include "runpacketfiledialog.h"
#include "kfxversion.h"
#include "settings.h"
#include "ui_runpacketfiledialog.h"

#include <QDir>
#include <QStringListModel>

RunPacketFileDialog::RunPacketFileDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RunPacketFileDialog)
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Hide notice about packetsave being disabled when loading a packet when
    // packetsave works during packetload or packetsave is not enabled
    if (KfxVersion::hasFunctionality("packetsave_while_packetload") || Settings::getLauncherSetting("GAME_PARAM_PACKET_SAVE_ENABLED").toBool() == false) {
        ui->infoPacketSaveLabel->hide();
    }

    // Get packet files
    QDir dir(QCoreApplication::applicationDirPath());
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList() << "*.pck");
    QStringList pckFiles = dir.entryList();

    // Load packet files into listview
    QStringListModel *model = new QStringListModel();
    model->setStringList(pckFiles);
    ui->listView->setModel(model);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Disable start button at start
    ui->startButton->setDisabled(true);

    // Enable/disable start button if a row is selected
    connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RunPacketFileDialog::updateStartButton);
}

RunPacketFileDialog::~RunPacketFileDialog()
{
    delete ui;
}

QString RunPacketFileDialog::getPacketFileName()
{
    return this->packetFileName;
}

void RunPacketFileDialog::updateStartButton()
{
    bool hasSelection = ui->listView->selectionModel()->hasSelection();
    ui->startButton->setEnabled(hasSelection);
}

void RunPacketFileDialog::on_cancelButton_clicked()
{
    qDebug() << "Closing dialog";
    this->close();
}

void RunPacketFileDialog::on_startButton_clicked()
{
    qDebug() << "Start button clicked";

    // Get selection
    QModelIndexList selected = ui->listView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        qDebug() << "Nothing was selected";
        return;
    }

    // Get packetfile string
    QString packetFileNameString = selected.first().data().toString();
    qDebug() << "Selected packetfile:" << packetFileNameString;

    // Load packetfile
    this->packetFileName = packetFileNameString;

    // Accept dialog
    // This will start the game
    this->accept();
}
