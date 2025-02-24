#pragma once

#include <QFile>
#include <QString>
#include <QMetaEnum>

class KfxVersion : public QObject {
    Q_OBJECT

public:

    enum ReleaseType
    {
        UNKNOWN,
        STABLE,
        ALPHA,
        PROTOTYPE,
    };
    Q_ENUM(ReleaseType)

    inline static ReleaseType getReleaseTypefromString(const QString &str) {
        QMetaEnum metaEnum = QMetaEnum::fromType<ReleaseType>();
        int value = metaEnum.keyToValue(str.toUtf8().toUpper().constData());
        return (value == -1) ? UNKNOWN : static_cast<ReleaseType>(value);
    }

    struct Version
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        int build = 0;
        ReleaseType type = ReleaseType::UNKNOWN;
        QString string;
    };

    struct VersionInfo
    {
        QString version;
        QString downloadUrl;
        KfxVersion::ReleaseType type;
    };

    static Version currentVersion;

    static QString getVersionString(QFile binary);
    static QString getVersionStringFromAppDir();
    static Version getVersionFromString(QString versionString);

    static bool loadCurrentVersion();

    static bool isVersionLowerOrEqual(const QString &fileVersion, const QString &currentVersion);
    static bool isNewerVersion(const QString &fileVersion, const QString &currentVersion);

    static std::optional<VersionInfo> getLatestVersion(ReleaseType type);
    static std::optional<QMap<QString, QString>> getGameFileMap(ReleaseType type, QString version);
};
