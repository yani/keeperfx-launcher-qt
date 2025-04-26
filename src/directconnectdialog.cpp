#include "directconnectdialog.h"
#include "ui_directconnectdialog.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "kfxversion.h"

DirectConnectDialog::DirectConnectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DirectConnectDialog)
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Port validator: allows only numbers (0-65535)
    ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));

    // IP validator: allows IPv4 characters, and IPv6 characters if KeeperFX has that functionality
    ui->ipLineEdit->setValidator(new QRegularExpressionValidator(
        KfxVersion::hasFunctionality("enet_ipv6_support") ?
        QRegularExpression("^[0-9a-fA-F:.]*$") :
        QRegularExpression("^[0-9.]*$")
        , this));
}

DirectConnectDialog::~DirectConnectDialog()
{
    delete ui;
}

QString DirectConnectDialog::getIp(){
    return this->ip;
}

int DirectConnectDialog::getPort(){
    return this->port;
}

void DirectConnectDialog::on_sendButton_clicked()
{
    // Get variables
    QString ipString(ui->ipLineEdit->text());
    int portNumber = ui->portLineEdit->text().toInt();
    QHostAddress ipHostAddress(ipString);

    // Make sure IP is valid
    if(ipHostAddress.isNull() || ipHostAddress.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol){
        QMessageBox::warning(this, tr("Direct Connect", "MessageBox Title"), tr("Invalid IP address", "MessageBox Text"));
        return;
    }

    // Check if IP is IPv4 if IPv6 is not supported
    if(KfxVersion::hasFunctionality("enet_ipv6_support") == false){
        if(ipHostAddress.protocol() == QAbstractSocket::IPv6Protocol){
            QMessageBox::warning(this, tr("Direct Connect", "MessageBox Title"), tr("IPv6 addresses are not supported.", "MessageBox Text"));
            return;
        }
    }

    // Return to main launcher window which will start the game
    this->ip = ipString;
    this->port = portNumber;
    this->accept();
}

void DirectConnectDialog::on_cancelButton_clicked()
{
    this->close();
}

