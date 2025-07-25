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

    static QString getUnearthBinaryPath()
    {
        // File path
        QString unearthBinaryPath(QCoreApplication::applicationDirPath() + QDir::separator() + "unearth" + QDir::separator() + "unearth.x86_64");
#ifdef Q_OS_WINDOWS
        // Add .exe to windows path
        unearthBinaryPath.append(".exe");
#endif
        return unearthBinaryPath;
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
