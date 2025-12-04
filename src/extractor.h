#pragma once

#include <QObject>
#include <QFile>
#include <QString>

class Extractor : public QObject
{
    Q_OBJECT

public:
    explicit Extractor(QObject *parent = nullptr);
    void extract(QFile *archiveFile, QString outputDir);

signals:
    void progress(uint64_t processedSize);
    void extractComplete();
    void extractFailed(const QString &error);

};
