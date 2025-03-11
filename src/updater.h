#pragma once

#include <functional>

#include <QObject>
#include <QFile>
#include <QString>

class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(QObject *parent = nullptr);
    void updateFromArchive(QFile *archiveFile);

signals:
    void progress(uint64_t processedSize);
    void updateComplete();
    void updateFailed(const QString &error);

};
