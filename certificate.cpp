#include "certificate.h"

#include <QUrl>
#include <QList>
#include <QSslCertificate>
#include <QSslError>

#include <LIEF/PE.hpp>

#include <stdexcept> // For std::runtime_error

QList<QSslCertificate> Certificate::certificateList;

void Certificate::loadAll()
{
    // Certificate list variable
    QList<QSslCertificate> certList;

    // Certificates to load
    QStringList certFiles = {
        ":/res/cert/release_certificate_2025.cer",
        ":/res/cert/test_certificate.cer",
    };

    // Loop through and load each certificate
    for (const QString &certFilePath : certFiles) {
        QFile certFile(certFilePath);
        if (certFile.open(QIODevice::ReadOnly)) {
            QByteArray certData = certFile.readAll();
            certList.append(QSslCertificate(certData));
            qDebug() << "Loaded certificate:" << certFilePath;
        } else {
            qWarning() << "Failed to open certificate:" << certFilePath;
            throw std::runtime_error("Failed to load certificates");
        }
    }

    Certificate::certificateList = certList;
}

bool Certificate::verify(QFile &file)
{
    // Load certificates if they are not loaded yet
    if (Certificate::certificateList.isEmpty()) {
        Certificate::loadAll();
    }

    // Make sure we have a certificate to use for verification
    if (Certificate::certificateList.isEmpty()) {
        qWarning() << "No certificates for verification";
        return false;
    }

    // Get filepath
    QString filePath = file.fileName();
    qDebug() << "Verifying:" << filePath;

    try {

        std::unique_ptr<LIEF::PE::Binary> binary = LIEF::PE::Parser::parse(filePath.toStdString());
        if (binary == nullptr) {
            qWarning() << "Failed to parse PE file.";
            return false;
        }

        // Iterate through the digital signatures
        for (const LIEF::PE::Signature& signature : binary->signatures()) {

            // Iterate through the certificates in the signature
            for (const LIEF::PE::x509& liefCert : signature.certificates()) {

                QByteArray certData(reinterpret_cast<const char*>(liefCert.raw().data()), static_cast<int>(liefCert.raw().size()));
                QSslCertificate signedCertificate(certData);

                // Compare to our own list of valid certificates
                for (const QSslCertificate &cert : Certificate::certificateList) {
                    if (signedCertificate == cert) {

                        // Success
                        qDebug() << "Certificate verified successfully.";
                        return true;
                    }
                }
            }
        }

        // Fail
        qWarning() << "No matching certificate found.";
        return false;

    } catch (const std::exception &e) {
        qWarning() << "Exception occurred while verifying PE file:" << e.what();
        return false;
    }
}

bool Certificate::verify(QString filePath)
{
    QFile file(filePath);
    return Certificate::verify(file);
}

bool Certificate::verify(QUrl fileUrl)
{
    QFile file(fileUrl.toLocalFile());  // Convert QUrl to QFile path
    return Certificate::verify(file);
}
