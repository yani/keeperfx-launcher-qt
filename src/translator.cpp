#include "translator.h"

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QString>

Translator::Translator(QObject *parent)
    : QTranslator(parent) {
    translations = QMap<QString, QString>();
}

void Translator::loadPoTranslations(const QString &languageCode)
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
    bool inMsgid = false;
    bool inMsgstr = false;

    int translationsLoaded = 0;

    while (!in.atEnd()) {
        line = in.readLine().trimmed();

        if (line.startsWith("msgid")) {
            if (!msgid.isEmpty() && !msgstr.isEmpty()) {
                translations[msgid] = msgstr;
                translationsLoaded++;
                msgid.clear();
                msgstr.clear();
            }
            msgid = line.mid(7).chopped(1);
            inMsgid = true;
            inMsgstr = false;
        } else if (line.startsWith("msgstr")) {
            msgstr = line.mid(8).chopped(1);
            inMsgid = false;
            inMsgstr = true;
        } else if (inMsgid && line.startsWith("\"")) {
            msgid += line.mid(1).chopped(1);
        } else if (inMsgstr && line.startsWith("\"")) {
            msgstr += line.mid(1).chopped(1);
        }
    }

    if (!msgid.isEmpty() && !msgstr.isEmpty()) {
        translations[msgid] = msgstr;
        translationsLoaded++;
    }

    qDebug() << "Translations loaded:" << translationsLoaded;
}

QString Translator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const {
    QString source(sourceText);
    if (translations.contains(source)) {
        QString output(translations.value(source));
        if(output.isEmpty() == false){
            return translations.value(source);
        }
    }
    return source;
}
