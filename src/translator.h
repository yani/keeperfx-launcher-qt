#pragma once

#include <QTranslator>
#include <QString>
#include <QMap>

class Translator : public QTranslator {
    Q_OBJECT

public:
    explicit Translator(QObject *parent = nullptr);

    void loadLanguage(const QString &languageCode);
    void loadPoFile(const QString &poFilePath);

    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1) const override;

private:
    QMap<QString, QString> translations;

};
