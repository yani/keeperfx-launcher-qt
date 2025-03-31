#include "translator.h"

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QString>

Translator::Translator(QObject *parent)
    : QTranslator(parent) {
    translations = QMap<QString, QString>();
}

void Translator::loadPotTranslations(const QString &languageCode)
{
    QString fileName = QString(":/i18n/i18n/translations_%1.po").arg(languageCode);
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open translation file:" << fileName;
        return;
    }

    translations.clear();
    QTextStream in(&file);
    QString line;
    QString msgid;
    QString msgstr;

    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        if (line.startsWith("msgid")) {
            msgid = line.mid(7).chopped(1);
        } else if (line.startsWith("msgstr")) {
            msgstr = line.mid(8).chopped(1);
            translations[msgid] = msgstr;
        }
    }
}

QString Translator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const {
    QString source(sourceText);
    if (translations.contains(source)) {
        return translations.value(source);
    }
    return source;
}
