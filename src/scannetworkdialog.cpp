#include "enetlanscanner.h"
#include "scannetworkdialog.h"
#include "ui_scannetworkdialog.h"

ScanNetworkDialog::ScanNetworkDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ScanNetworkDialog)
    , scanner(new EnetLanScanner)
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
    connect(&scanner, &EnetLanScanner::serverFound, this, &ScanNetworkDialog::handleServerFound);
    connect(&scanner, &EnetLanScanner::scanProgress, this, &ScanNetworkDialog::handleScanProgress);
    connect(&scanner, &EnetLanScanner::scanComplete, this, &ScanNetworkDialog::handleScanComplete);

    // Enable/disable connect button if a row is selected
    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &ScanNetworkDialog::updateConnectButton);

    // Hardcode 5556 for now
    this->port = 5556;
}

ScanNetworkDialog::~ScanNetworkDialog()
{
    delete ui;
}

QString ScanNetworkDialog::getIp(){
    return this->ip;
}

int ScanNetworkDialog::getPort(){
    return this->port;
}

void ScanNetworkDialog::on_cancelButton_clicked()
{
    this->close();
}

void ScanNetworkDialog::on_scanButton_clicked()
{
    // Disable buttons
    ui->scanButton->setDisabled(true);
    ui->connectButton->setDisabled(true);

    // Clear table
    // Removes all rows without clearing headers
    ui->tableWidget->setRowCount(0);

    // Start the scan
    scanner.startScan(this->port);
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

    // Enable scan button again
    ui->scanButton->setDisabled(false);

    // Clear progress bar
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setFormat("");
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
