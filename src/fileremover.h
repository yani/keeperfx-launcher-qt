#pragma once

#include "kfxversion.h"

#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>

class FileRemover
{
public:
    static QStringList processFile(QFile &file, const QString &currentVersion);

private:
    static bool isVersionLowerOrEqual(const QString &fileVersion, const QString &currentVersion);
};

QStringList FileRemover::processFile(QFile &file, const QString &currentVersion)
{
    // Variables
    QStringList filesToCheck;
    QString fileVersion;
    QStringList existingFiles;
    QDir baseDir(QCoreApplication::applicationDirPath());

    // Open the file
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return filesToCheck;
    }

    // Loop trough the file
    QTextStream in(&file);
    while (!in.atEnd()) {
        // Get the line
        QString line = in.readLine().trimmed();

        // Make sure this line is valid
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }

        // Check if this line is a version
        if (line.startsWith("[") && line.endsWith("]")) {
            QString versionString = line.mid(1, line.length() - 2);

            // Check if this is a valid version string
            static const QRegularExpression regex("^\\d+\\.\\d+(\\.\\d+){0,2}$");
            if (regex.match(versionString).hasMatch() == false) {

                // Invalid version string
                qWarning() << "Invalid version in" << file.fileName() << ":" << versionString;
                fileVersion = "999.999.999.999";
                continue;
            }

            // Set valid version string
            fileVersion = versionString;
            continue;
        }

        // Check if given version is lower or equal to our KeeperFX version
        if (!KfxVersion::isVersionLowerOrEqual(fileVersion, currentVersion)) {
            continue;
        }

        // Normalize path
#ifdef Q_OS_WINDOWS
        QString normalizedPath = QDir::cleanPath(line).replace("/", "\\");
        if (normalizedPath.startsWith("\\")) {
            normalizedPath.remove(0, 1);
        }
#else
        QString normalizedPath = QDir::cleanPath(line).replace("\\", "/");
        if (normalizedPath.startsWith("/")) {
            normalizedPath.remove(0, 1);
        }
#endif

        // Remember file to be checked
        filesToCheck.append(normalizedPath);
    }

    // Check if files that should be removed exist
    for (int i = 0; i < filesToCheck.size(); ++i) {
        const QString &filePath = filesToCheck.at(i);
        if (baseDir.exists(filePath)) {
            existingFiles.append(filePath);
        }
    }

    // Return the files to be removed
    return existingFiles;
}
