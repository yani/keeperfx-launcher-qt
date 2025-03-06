#include "kfxversion.h"
#include "apiclient.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QJsonObject>

#include <LIEF/PE.hpp>

KfxVersion::VersionInfo KfxVersion::currentVersion;

QString KfxVersion::getVersionString(QFile binary){

    // Make sure this binary file exists
    if(binary.exists() == false){
        return QString();
    }

    // Get filepath
    QString filePath = binary.fileName();
    qDebug() << "Checking app version for file:" << filePath;

    // Parse the PE file using LIEF
    // We use a library here instead of Windows calls to be consistent accross platforms
    // TODO: When we release a unix version of KeeperFX we should parse ELF data instead
    //       But for now we use the Windows binary with Wine so we should just read the PE data
    std::unique_ptr<LIEF::PE::Binary> peFile = LIEF::PE::Parser::parse(filePath.toStdString());

    // Check if PE file parser is made
    if (!peFile) {
        qDebug() << "Error: Unable to parse PE file:" << filePath;
        return QString();
    }

    // Check if the PE file has resources
    if (!peFile->has_resources()) {
        qDebug() << "Error: No resources found in the PE file.";
        return QString();
    }

    // Get the resource manager
    auto manager = peFile->resources_manager();
    if(!manager){
        qDebug() << "Error: Failed to get PE file resource manager.";
        return QString();
    }

    // Get the versions
    auto versions = manager->version();
    if (versions.empty()) {
        qDebug() << "Error: No version information found in resources.";
        return QString();
    }

    // Capture the first version entry (assuming there's at least one)
    std::ostringstream oss;
    oss << versions.front(); // Get the first ResourceVersion

    QString resourcesInfo = QString::fromStdString(oss.str());

    qDebug() << "RESOURCE INFO:" << resourcesInfo;

    // Define regex pattern to match 'ProductVersion'
    QRegularExpression regex (R"(ProductVersion:\s*(.+?)\s*?[\\\n])");
    QRegularExpressionMatch match = regex.match(resourcesInfo);

    // Get regex match
    if (match.hasMatch()) {
        qDebug() << match.captured(1);
        return match.captured(1);
    } else {
        qDebug() << "Error: Version not found in PE file";
    }

    return QString();
}

QString KfxVersion::getVersionStringFromAppDir()
{
    return KfxVersion::getVersionString(
        QFile(QCoreApplication::applicationDirPath() + "/keeperfx.exe")
    );
}

KfxVersion::VersionInfo KfxVersion::getVersionFromString(QString versionString)
{
    VersionInfo versionInfo;
    versionInfo.fullString = versionString;

    // Use regex to get the version from the string
    // Catches 1.2.3 and 1.2.3.4
    QRegularExpression regex (R"([0-9]+\.[0-9]+\.[0-9]+(?:\.[0-9]+)?)");
    QRegularExpressionMatch match = regex.match(versionString);

    // Check if regex has a match
    if (match.hasMatch() == false) {
        return versionInfo;
    }

    // Get version
    versionInfo.version = match.captured(0);

    // Get the type of the release
    if (versionInfo.version == versionString) {
        versionInfo.type = KfxVersion::ReleaseType::STABLE;
        // Make sure only x.y.z are taken from stable version
        versionInfo.version = versionInfo.version.split('.').mid(0, 3).join('.');
        versionInfo.fullString = versionInfo.version;
    } else if (versionString.toLower().contains("alpha")) {
        versionInfo.type = KfxVersion::ReleaseType::ALPHA;
    } else if (versionString.toLower().contains("prototype")) {
        versionInfo.type = KfxVersion::ReleaseType::PROTOTYPE;
    } else {
        versionInfo.type = KfxVersion::ReleaseType::UNKNOWN;
    }

    return versionInfo;
}

bool KfxVersion::loadCurrentVersion()
{
    // Get the version
    QString versionString = getVersionStringFromAppDir();
    VersionInfo version = getVersionFromString(versionString);

    // Check if version is valid
    if(version.type == ReleaseType::UNKNOWN){
        return false;
    }

    // Remember the current Kfx version
    currentVersion = version;

    return true;
}

bool KfxVersion::isVersionLowerOrEqual(const QString &version1, const QString &version2) {

    // Get version parts
    QStringList version1Parts = version1.split(".");
    QStringList version2Parts = version2.split(".");

    // Normalize version parts to equal sizes
    int maxLength = qMax(version1Parts.size(), version2Parts.size());
    while (version1Parts.size() < maxLength) version1Parts.append("0");
    while (version2Parts.size() < maxLength) version2Parts.append("0");

    // Loop trough the version parts
    for (int i = 0; i < maxLength; ++i) {

        // Check if version is newer or older
        if (version1Parts[i].toInt() < version2Parts[i].toInt()) {
            return true;
        } else if (version1Parts[i].toInt() > version2Parts[i].toInt()) {
            return false;
        }
    }

    // Versions are equal
    return true;
}

bool KfxVersion::isNewerVersion(const QString &version1, const QString &version2)
{
    // Get version parts
    QStringList version1Parts = version1.split(".");
    QStringList version2Parts = version2.split(".");

    // Normalize version parts to equal sizes
    int maxLength = qMax(version1Parts.size(), version2Parts.size());
    while (version1Parts.size() < maxLength) version1Parts.append("0");
    while (version2Parts.size() < maxLength) version2Parts.append("0");

    // Loop trough the version parts
    for (int i = 0; i < maxLength; ++i) {

        // Check if version is newer
        if (version1Parts[i].toInt() > version2Parts[i].toInt()) {
            return true;
        }
    }

    return false;
}

std::optional<KfxVersion::VersionInfo> KfxVersion::getLatestVersion(KfxVersion::ReleaseType type)
{
    // Only check version for stable and alpha
    if (type != KfxVersion::ReleaseType::STABLE && type != KfxVersion::ReleaseType::ALPHA) {
        return std::nullopt;
    }

    // Variables
    QString version;
    QString downloadUrl;
    QString fullVersionString;

    // Handle stable
    if (type == KfxVersion::ReleaseType::STABLE) {
        // Get release
        QJsonObject stableRelease = ApiClient::getLatestStable();
        if (stableRelease.isEmpty()) {
            return std::nullopt;
        }

        // Set vars
        version = stableRelease["version"].toString();
        downloadUrl = stableRelease["download_url"].toString();
        fullVersionString = version;
    }

    // Handle alpha
    if (type == KfxVersion::ReleaseType::ALPHA) {
        // Get release
        QJsonObject alphaRelease = ApiClient::getLatestAlpha();
        if (alphaRelease.isEmpty()) {
            return std::nullopt;
        }

        // Set vars
        version = alphaRelease["version"].toString();
        downloadUrl = alphaRelease["download_url"].toString();
        fullVersionString = version + " Alpha";
    }

    // Return latest version information
    return VersionInfo{
        .type = type,
        .version = version,
        .fullString = fullVersionString,
        .downloadUrl = downloadUrl
    };
}

std::optional<QMap<QString, QString>> KfxVersion::getGameFileMap(KfxVersion::ReleaseType type, QString version)
{
    // Only check version for stable and alpha
    if (type != KfxVersion::ReleaseType::STABLE && type != KfxVersion::ReleaseType::ALPHA) {
        return std::nullopt;
    }

    // Get release file map
    auto fileMap = ApiClient::getGameFileList(type, version);
    if (!fileMap) {
        return std::nullopt;
    }

    return fileMap;
}
