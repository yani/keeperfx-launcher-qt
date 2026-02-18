#include "modmanager.h"
#include "mod.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

ModManager::ModManager()
{
    qDebug() << "Initializing ModManager";

    // Check if the save file dir exists
    QDir modsDir(QCoreApplication::applicationDirPath() + "/mods");
    if (modsDir.exists() == false || modsDir.isReadable() == false) {
        qWarning() << "Mods directory does not exist or is not readable:" << modsDir.filesystemAbsolutePath();
        return;
    }

    // Get mod folders
    QStringList modFolders = modsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (modFolders.isEmpty()) {
        qInfo() << "No mods found";
        return;
    }

    // Loop trough all folders
    for (const QString &modFolderName : std::as_const(modFolders)) {
        // Ignore directories that start with a dot
        // Linux (POSIX) filesystems use this naming scheme for hidden folders
        // Note: dot and dotdot dirs are already filtered before this check
        if (modFolderName.startsWith(".")) {
            continue;
        }

        // Try to load this mod
        Mod *mod = new Mod(modsDir.absoluteFilePath(modFolderName));

        // Put mod into mod list
        if(mod->isValid()){
            this->mods.append(mod);
        }
    }

    // Check if mods have been found
    int count = this->mods.count();
    if (count == 0) {
        qInfo() << "0 mods found";
    } else {
        // Log the mods to the console
        qInfo().noquote() << QString("%1 mods found:").arg(count);
        for (const Mod *mod : std::as_const(this->mods)) {
            qInfo().noquote() << QString("- %1").arg(mod->toString());
        }
    }

    // Open the load order file
    // This also opens a handle even if the file does not exist to create it instead
    QFile loadOrderFile(QCoreApplication::applicationDirPath() + QDir::separator() + "mods" + QDir::separator() + "load_order.cfg");
    if (!loadOrderFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qWarning() << "Failed to open mod load order file:" << loadOrderFile.errorString();
        return;
    }

    // The section map to place the mods in their correct list
    QHash<QString, QList<Mod*>*> sectionMap {
        { "after_base", &this->modsAfterBase },
        { "after_campaign", &this->modsAfterCampaign },
        { "after_map", &this->modsAfterMap }
    };

    // The list to populate
    QList<Mod*>* sectionList;

    // Loop trough the file
    QTextStream in(&loadOrderFile);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty())
            continue;

        if (line.startsWith(';'))
            continue;

        // Line is the start of a section
        if (line.startsWith('[') && line.endsWith(']')) {

            QString section = line.mid(1, line.length() - 2);
            sectionList = sectionMap.value(section, nullptr);

            if(!sectionList){
                qWarning() << "Invalid mod load order section:" << section;
                continue;
            } else {
                qInfo() << "Mod load order:" << section;
            }

            continue;
        }

        // Check if we are in a valid section
        if(!sectionList){
            continue;
        }

        // Try to load this mod
        Mod *mod = new Mod(modsDir.absoluteFilePath(line));
        if(mod->isValid()){

            // Add mod to correct section list
            // It's important that it's appended at the end and that the section lists are split
            sectionList->append(mod);
            qInfo() << "   -" << mod->name;
        }
    }
    /*
    const QStringList sections = settings.childGroups();
    for (const QString& section : sections) {

        QList<Mod*>* sectionList = sectionMap.value(section, nullptr);
        if(!sectionList){
            qWarning() << "Invalid mod load order section:" << section;
            continue;
        } else {
            qInfo() << "Mod load order:" << section;
        }

        // Open this settings section
        settings.beginGroup(section);

        // Loop trough all defined mods
        for (const QString& modName : settings.childKeys()) {

            // Try to load this mod
            Mod *mod = new Mod(modsDir.absoluteFilePath(modName));

            if(mod->isValid()){
                sectionList->append(mod);
                qInfo() << "   -" << modName;
            } else {
                qWarning() << "Mod does not have a folder:" << modName;
            }
        }

        // Close this settings section
        settings.endGroup();
    }*/
}
