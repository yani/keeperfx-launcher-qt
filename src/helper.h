#pragma once

#include <QVariant>
#include <QString>
#include <QObject>
#include <QProcess>

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
};
