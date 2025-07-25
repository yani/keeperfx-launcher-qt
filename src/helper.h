#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariant>

#ifdef Q_OS_WINDOWS
    #include <windows.h>
#endif

#include <LIEF/PE.hpp>

class Helper
{
public:
    static bool is64BitDLL(const std::string &dllPath)
    {
        try {
            auto pe = LIEF::PE::Parser::parse(dllPath);
            return LIEF::PE::Header::x86_64(pe->header().machine());
        } catch (const std::exception &e) {
            qWarning() << "LIEF error: " << e.what();
            return false; // Assume 32-bit or invalid file if parsing fails
        }
    }

    static bool is64BitDll(QString dllPath) { return Helper::is64BitDLL(dllPath.toStdString()); }

    static QFile getUnearthBinary()
    {
        const QString rootDir = QCoreApplication::applicationDirPath();
        const QStringList subdirs = {"unearth", "Unearth"};

#ifdef Q_OS_WINDOWS
        const QStringList filenames = {"unearth.exe"};
#else
        const QStringList filenames = {"unearth", "unearth.x86_64"};
#endif

        for (const QString &subdir : subdirs) {
            QDir dir(rootDir + QDir::separator() + subdir);
            if (!dir.exists())
                continue;

            const QStringList entries = dir.entryList(QDir::Files | QDir::NoSymLinks);
            for (const QString &entry : entries) {
                for (const QString &expected : filenames) {
                    if (entry.compare(expected, Qt::CaseInsensitive) == 0) {
                        return QFile(dir.absoluteFilePath(entry));
                    }
                }
            }
        }

        // Not found
        return QFile();
    }

#ifdef Q_OS_WINDOWS
    // Function to check if we are running under Wine
    static bool isRunningUnderWine()
    {
        HMODULE hModule = GetModuleHandleA("ntdll.dll");
        if (hModule) {
            // Check for Wine-specific function
            if (GetProcAddress(hModule, "wine_get_version")) {
                return true;
            }
        }
        return false;
    }

    // Function to get the Wine version as a QString
    static QString getWineVersion()
    {
        typedef const char *(__cdecl * wine_get_version_func)();
        HMODULE hModule = GetModuleHandleA("ntdll.dll");
        if (hModule) {
            wine_get_version_func wine_get_version = (wine_get_version_func) GetProcAddress(hModule, "wine_get_version");
            if (wine_get_version) {
                return QString::fromUtf8(wine_get_version());
            }
        }
        return QString();
    }

    // Function to get the Wine host machine name as a QString
    static QString getWineHostMachineName()
    {
        typedef const char *(__cdecl * wine_get_host_machine_name_func)();
        HMODULE hModule = GetModuleHandleA("ntdll.dll");
        if (hModule) {
            wine_get_host_machine_name_func wine_get_host_machine_name = (wine_get_host_machine_name_func) GetProcAddress(hModule, "wine_get_host_machine_name");
            if (wine_get_host_machine_name) {
                return QString::fromUtf8(wine_get_host_machine_name());
            }
        }
        return QString();
    }
#endif
};
