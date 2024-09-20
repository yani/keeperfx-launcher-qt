#include "kfxversion.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include <LIEF/PE.hpp>

KfxVersion::Version KfxVersion::currentVersion;

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

    // Capture the resource manager details
    std::ostringstream oss;
    oss << *manager->version(); // not the best way but meh
    QString resourcesInfo = QString::fromStdString(oss.str());

    // Define regex pattern to match 'ProductVersion'
    QRegularExpression regex (R"('ProductVersion':\s*'([^']*)')");
    QRegularExpressionMatch match = regex.match(resourcesInfo);

    // Get regex match
    if (match.hasMatch()) {
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

KfxVersion::Version KfxVersion::getVersionFromString(QString versionString)
{
    // Use regex to get version parts of the string
    QRegularExpression regex (R"(([0-9]+)\.([0-9]+)\.([0-9]+)(?:\.([0-9]+)?))");
    QRegularExpressionMatch match = regex.match(versionString);

    // Check if regex has a match
    if (match.hasMatch() == false) {
        return Version{};
    }

    // Create the version
    Version version = {
        .major = match.captured(1).toInt(),
        .minor = match.captured(2).toInt(),
        .patch = match.captured(3).toInt(),
        .type = ReleaseType::STABLE,
        .string = versionString
    };

    // Get the build
    if(match.captured(4).isEmpty() == false){
        version.build = match.captured(4).toInt();
    }

    // Get the type of the release
    if(versionString.toLower().contains("alpha")){
        version.type = KfxVersion::ReleaseType::ALPHA;
    } else if(versionString.toLower().contains("prototype")){
        version.type = KfxVersion::ReleaseType::PROTOTYPE;
    }

    // Stable releases don't need the build version
    if(version.type == ReleaseType::STABLE){
        version.string =
            QString::number(version.major)
            + "."
            + QString::number(version.minor)
            + "."
            + QString::number(version.patch);
    }

    return version;
}

bool KfxVersion::loadCurrentVersion()
{
    // Get the version
    QString versionString = getVersionStringFromAppDir();
    Version version = getVersionFromString(versionString);

    // Check if version is valid
    if(version.type == ReleaseType::UNKNOWN){
        return false;
    }

    // Remember the current Kfx version
    currentVersion = version;

    return true;
}
