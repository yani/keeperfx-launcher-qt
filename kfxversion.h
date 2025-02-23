#pragma once

#include <QFile>
#include <QString>

class KfxVersion
{
    enum ReleaseType
    {
        UNKNOWN,
        STABLE,
        ALPHA,
        PROTOTYPE,
    };

    struct Version
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        int build = 0;
        ReleaseType type = ReleaseType::UNKNOWN;
        QString string;
    };

public:

    static Version currentVersion;

    static QString getVersionString(QFile binary);
    static QString getVersionStringFromAppDir();
    static Version getVersionFromString(QString versionString);

    static bool loadCurrentVersion();

    static bool isVersionLowerOrEqual(const QString &fileVersion, const QString &currentVersion);
};
