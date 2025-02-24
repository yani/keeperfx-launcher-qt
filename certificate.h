#pragma once

#include <QUrl>
#include <QString>
#include <QFile>
#include <QList>
#include <QSslCertificate>

class Certificate
{
public:
    static bool verify(QFile &file);
    static bool verify(QString filePath);
    static bool verify(QUrl fileUrl);

    static QList<QSslCertificate> certificateList;
    static void loadAll();
};
