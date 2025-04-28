#include "translator.h"

#include "launcheroptions.h"

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QString>
#include <QTextDocument>

Translator::Translator(QObject *parent)
    : QTranslator(parent) {
    translations = QMap<QString, QString>();
}

void Translator::loadPoTranslations(const QString &languageCode)
{
    // Translation filepath in resources
    QString translationFilePath = QString(":/i18n/i18n/translations_%1.po").arg(languageCode);

    // Check if custom translation file is set
    if (LauncherOptions::isSet("translation-file")) {
        translationFilePath = LauncherOptions::getValue("translation-file");
        qDebug() << "Loading translation file directly:" << translationFilePath;
    }

    // Open translation file
    QFile file(translationFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open translation file:" << translationFilePath;
        return;
    }

    // Clear current translations
    translations.clear();

    // Variables
    QTextStream in(&file);
    QString line;
    QString msgid;
    QString msgstr;
    bool inMsgid = false;
    bool inMsgstr = false;
    int translationsLoaded = 0;

    // Loop trough PO file
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
    // Get source as string
    QString source(sourceText);

    // Check if this source has a translation
    if (translations.contains(source)) {
        QString output(translations.value(source));
        if (output.isEmpty() == false) {
            return output;
        }
    }

    // Check if this source has a translation that is HTML escaped
    if (translations.contains(source.toHtmlEscaped())) {
        QString output(translations.value(source.toHtmlEscaped()));
        if (output.isEmpty() == false) {
            QTextDocument outputDoc;
            outputDoc.setHtml(output);
            return outputDoc.toPlainText();
        }
    }

    return source;
}
