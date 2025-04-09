#include "enetlanscanner.h"
#include "scannetworkdialog.h"
#include "ui_scannetworkdialog.h"

ScanNetworkDialog::ScanNetworkDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ScanNetworkDialog)
    , scanner(new EnetLanScanner(this))
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Setup table
    ui->tableWidget->setColumnCount(2);
    QStringList headers;
    headers << tr("IP Address") << tr("Hostname");
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Disable connect button at start
    ui->connectButton->setDisabled(true);

    // Connect scanner signals to slots
    connect(scanner, &EnetLanScanner::serverFound, this, &ScanNetworkDialog::handleServerFound);
    connect(scanner, &EnetLanScanner::scanProgress, this, &ScanNetworkDialog::handleScanProgress);
    connect(scanner, &EnetLanScanner::scanComplete, this, &ScanNetworkDialog::handleScanComplete);

    // Enable/disable connect button if a row is selected
    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &ScanNetworkDialog::updateConnectButton);

    // Hardcode 5556 for now
    this->port = 5556;
}

ScanNetworkDialog::~ScanNetworkDialog()
{
    // Make sure scanner is stopped
    scanner->stopScan();

    // Delete the scanner
    delete scanner;
    scanner = nullptr;

    // Delete UI
    delete ui;
}

QString ScanNetworkDialog::getIp(){
    return this->ip;
}

int ScanNetworkDialog::getPort(){
    return this->port;
}

void ScanNetworkDialog::on_closeButton_clicked()
{
    this->close();
}

void ScanNetworkDialog::on_scanButton_clicked()
{
    if (isScanning == false) { // "Scan"
        isScanning = true;

        // Change scan button into Stop button
        ui->scanButton->setText(tr("Stop Scan"));

        // Connect button during scan
        ui->connectButton->setDisabled(true);

        // Clear table
        // Removes all rows without clearing headers
        ui->tableWidget->setRowCount(0);

        // Show that we are scanning
        ui->progressBar->setFormat(tr("Scanning..."));
        QCoreApplication::processEvents(); // Force format update

        // Start the scan
        scanner->startScan(this->port);

    } else { // "Stop Scan"
        isScanning = false;

        // Stop the scan
        scanner->stopScan();

        // Change Stop button into scan button again
        ui->scanButton->setText(tr("Scan"));

        // Clear progress bar
        ui->progressBar->setValue(0);
        ui->progressBar->setMaximum(1);
        ui->progressBar->setFormat("");
    }
}

void ScanNetworkDialog::handleServerFound(const QString &ip, const QString &hostname)
{
    qDebug() << "Server found:" << ip << "Hostname:" << hostname;

    // Add row to table
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(ip));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(hostname));
}

void ScanNetworkDialog::handleScanProgress(int scanned, int total)
{
    // Ignore progress if user stopped the scan
    if (isScanning == false) {
        return;
    }

    // Setup progress bar
    if(ui->progressBar->maximum() != total){
        ui->progressBar->setMaximum(total);
        ui->progressBar->setFormat(QString(tr("Scanning") + ": %p% (%v/%m)"));
    }

    // Update progress bar
    ui->progressBar->setValue(scanned);
}

void ScanNetworkDialog::handleScanComplete()
{
    qDebug() << "ENET lobby Scan complete";

    // Change Stop button into scan button again
    ui->scanButton->setText(tr("Scan"));

    // Clear progress bar
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setFormat("");

    // Remember that we are not scanning anymore
    isScanning = false;
}

void ScanNetworkDialog::on_connectButton_clicked()
{
    // Get selected row
    QList<QTableWidgetItem *> selectedItems = ui->tableWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        qWarning() << "Unable to find row";
        return;
    }

    // Get IP from selected row
    int row = selectedItems.first()->row();  // Get the row index
    QString ipAddress = ui->tableWidget->item(row, 0)->text();  // Get the IP from column 0
    qDebug() << "Selected IP Address:" << ipAddress;
    this->ip = ipAddress;

    // Close dialog
    this->accept();
}

void ScanNetworkDialog::updateConnectButton()
{
    // Check if there is a selected row
    bool hasSelection = !ui->tableWidget->selectedItems().isEmpty();
    ui->connectButton->setEnabled(hasSelection);
}
