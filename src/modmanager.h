#pragma once

#include "kfxversion.h"
#include "mod.h"

#include <QCoreApplication>
#include <QDir>

class ModManager
{
public:
    ModManager();

    static bool isModsFunctionalityAvailable()
    {
        // Make sure our current version supports mods
        if (KfxVersion::hasFunctionality("mod_support") == false) {
            qInfo() << "KeeperFX game version is too low for mod support";
            return false;
        }

        // Return if mods folder is available
        return ModManager::isModsFolderAvailable();
    }

    QList<Mod *> modsAfterBase;
    QList<Mod *> modsAfterCampaign;
    QList<Mod *> modsAfterMap;

private:

    QList<Mod *> mods;

    static bool isModsFolderAvailable()
    {
        // Get mods directory
        const QString rootDir = QCoreApplication::applicationDirPath();
        QDir dir(rootDir + QDir::separator() + "mods");

        // Make sure mods folder exists and is readable
        if (dir.exists() == false || dir.isReadable() == false) {
            qWarning() << "/mods directory does not exist";
            return false;
        }

        // Check if mods folder is writeable
        QFileInfo dirInfo(dir.absolutePath());
        if (dirInfo.isWritable() == false) {
            qWarning() << "/mods directory is not writeable";
            return false;
        }

        return true;
    }
};
