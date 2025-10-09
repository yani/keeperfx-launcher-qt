#include "modmanager.h"
#include "mod.h"

#include <QCoreApplication>
#include <QDir>

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
    for (const QString &modFolderName : modFolders) {
        // Ignore directories that start with a dot
        // Linux (POSIX) filesystems use this naming scheme for hidden folders
        // Note: dot and dotdot dirs are already filtered before this check
        if (modFolderName.startsWith(".")) {
            continue;
        }

        // Try to load this mod
        Mod *mod = new Mod(modsDir.absoluteFilePath(modFolderName));

        // Put mod into mod list
        this->mods.append(mod);
    }

    // Log how many mods have been found
    // Log results
    int count = this->mods.count();
    if (count == 0) {
        qInfo() << "0 mods found";
    } else {
        qInfo().noquote() << QString("%1 mods found:").arg(count);
        for (const Mod *mod : this->mods) {
            qInfo().noquote() << QString("- %1").arg(mod->toString());
        }
    }
}

QList<Mod *> ModManager::getMods()
{
    return this->mods;
}
